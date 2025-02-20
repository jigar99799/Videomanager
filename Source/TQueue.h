#ifndef TQUEUE_H
#define TQUEUE_H

#include <queue>
#include <stdexcept>
#include <iostream>
#include <type_traits> 
#include <shared_mutex>  
#include <mutex>    

#define DEFAULTQUEUESIZE 500

// Template definition with a non-type template parameter (iMaxSize) of type size_t
template <typename T, size_t iMaxSize = DEFAULTQUEUESIZE>  
class TQueue {
private:
    size_t m_iMaxSize = iMaxSize;
    std::queue<T> queue;        
    mutable std::shared_mutex mtx;     

    // Helper function to clean up memory if T is a pointer type
    void cleanup() {
        if constexpr (std::is_pointer_v<T>) {
            // If T is a pointer type, deallocate memory
            while (!queue.empty()) {

                T Tdata = queue.front();

                if(Tdata != nullptr)
                {
                    delete Tdata; 
                    Tdata = nullptr;
                }

                queue.pop();
            }
        } else {
            // If T is not a pointer, just clear the queue
            while (!queue.empty()) {
                queue.pop();
            }
        }
    }

    void Copyqueue(const TQueue& other)
    {
         // Deep copy: if T is a pointer, we need to allocate new objects
        if constexpr (std::is_pointer_v<T>) {
            std::queue<T> tempQueue = other.queue; 

            // Allocate new memory for each pointer in the original queue
            while (!tempQueue.empty()) {

                T Tdata = tempQueue.front();
                
                if(Tdata != nullptr)
                    queue.emplace(new typename std::remove_pointer<T>::type(*Tdata));
                else
					queue.emplace(nullptr);

                tempQueue.pop();
            }
        } else {
            // If T is not a pointer, perform a shallow copy
            queue = other.queue;
        }
    }

public:
    // Constructor: No need for maxSize since it is a template parameter
    TQueue() = default;

    // Destructor: Clean up memory if necessary
    ~TQueue() {
        cleanup();
    }

    // Implement deep copy constructor when T is a pointer
    TQueue(const TQueue& other) {
        std::unique_lock <std::mutex> lock(other.mtx); // Lock the other queue

        Copyqueue(other);
    }

    // Implement deep copy assignment operator when T is a pointer
    TQueue& operator=(const TQueue& other) {
        if (this == &other) return *this; 

        std::unique_lock<std::shared_mutex> lock1(mtx, std::defer_lock);  
        std::unique_lock<std::shared_mutex> lock2(other.mtx, std::defer_lock);
        std::lock(lock1, lock2); 

        cleanup();  

        Copyqueue(other);

        return *this;
    }

    // Move constructor
    TQueue(TQueue&& other) noexcept {
        std::unique_lock<std::shared_mutex> lock(other.mtx);

        queue = std::move(other.queue);

    }
    
    // Move assignment operator
    TQueue& operator=(TQueue&& other) noexcept {
        if (this == &other) return *this;

        std::unique_lock<std::shared_mutex> lock1(mtx, std::defer_lock);
        std::unique_lock<std::shared_mutex> lock2(other.mtx, std::defer_lock);

        std::lock(lock1, lock2);

        cleanup();

        queue = std::move(other.queue);

        return *this;

    }

    // Enqueue an element
    void enqueue(const T& item) {
        std::unique_lock<std::shared_mutex> lock(mtx); 

        // Check if the queue has reached its maximum size
        if (queue.size() >= m_iMaxSize) {
            throw std::runtime_error("Queue is full");
        }
        
        // If T is a pointer type, check if the item is nullptr before adding it
		if constexpr (std::is_pointer_v<T>) {
			if (item == nullptr) {
				throw std::runtime_error("Cannot insert nullptr into the queue: The queue does not support null pointers as valid elements.");
			}
		}

        // Push the item to the queue
        queue.emplace(item);
        
    }

    // Dequeue an element
    T dequeue() {
        std::unique_lock <std::shared_mutex> lock(mtx); 
        
        if (queue.empty()) {
            throw std::runtime_error("Queue is empty");
        }
        
        T item = std::move(queue.front()); 
        
        queue.pop(); 

        return item;
    }

    // Check if the queue is empty
    bool isEmpty() const {
        std::shared_lock <std::shared_mutex> lock(mtx); 
        return queue.empty();
    }

    // Clear the queue
    void clear() {
        std::unique_lock <std::shared_mutex> lock(mtx); 
        cleanup();
    }

    // Get the size of the queue
    size_t size() const {
        std::shared_lock <std::shared_mutex> lock(mtx); 
        return queue.size();
    }

    // Peek at the front element of the queue
    T front() const {
        std::shared_lock <std::shared_mutex> lock(mtx); 
        if (queue.empty()) {
            throw std::runtime_error("Queue is empty");
        }

        T item = std::move(queue.front());              

        return item;
    }

    // Pop an element (without returning it)
    void pop() {
        std::unique_lock <std::shared_mutex> lock(mtx); 
        if (queue.empty()) {
            throw std::runtime_error("Queue is empty");
        }
        queue.pop(); // Remove the front item
    }

    // Peek at the back element of the queue
    T back() const {
        std::shared_lock <std::shared_mutex> lock(mtx); 
        if (queue.empty()) {
            throw std::runtime_error("Queue is empty");
        }

        T item = std::move(queue.back());

        return item;
    }

    // Swap contents with another queue
    void swap(TQueue<T, iMaxSize>& other) {

        std::unique_lock<std::shared_mutex> lock1(mtx, std::defer_lock);  
		std::unique_lock<std::shared_mutex> lock2(other.mtx, std::defer_lock);

        std::lock(lock1, lock2); 
        
        std::swap(queue, other.queue);

    }

    // Access an element by index (not typical for a queue, but included as a function)
    T operator[](size_t index) const {
        std::shared_lock <std::shared_mutex> lock(mtx); 
        if (index >= queue.size()) {
            throw std::out_of_range("Index out of range");
        }

        // Create a temporary queue for accessing the element at a specific index
        std::queue<T> tempQueue = queue;
        for (size_t i = 0; i < index; ++i) {
            tempQueue.pop();
        }

        T item = std::move(queue.front());

        return item;
    }
};

#endif // TQUEUE_H
