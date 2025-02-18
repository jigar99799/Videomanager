#include <thread>
#include <condition_variable>
#include <atomic>
#include <random>
#include <string>

#include "RandomGenerator.h"
#include "JSONUtils.h"
#include "XMLUtils.h"
#include "TQueue.h"
#include "Logger.h"  
#include "Pipeline.h"
#include "MxPipelineManager.h"
constexpr int arraySize = 10;
const std::chrono::minutes RUN_DURATION(1);
std::chrono::steady_clock::time_point start_time;

// Thread-safe queues
TQueue<std::string> stringQueue;
TQueue<Pipeline> parsedQueue;

std::mutex parseGeneratedmtx;
std::mutex processParsedmtx;
std::condition_variable cvparseGenerate;
std::condition_variable cvprocessParsed;
std::atomic<bool> keepRunning(true);  // Atomic flag to control processing thread's running state

// Logger instance for logging output
Logger logger(Logger::OutputType::Console);  // You can also change to Logger::OutputType::File for file output

// Function to create JSON
void createJSON(const MediaStreamDevice& device) {
    std::string jsonString = JSONUtils::CreateJSON(device);
    logger.log("Generated JSON: \n" + jsonString);  // Log the generated JSON
}

// Function to create XML
void createXML(const MediaStreamDevice& device) {
    std::string xmlString = XMLUtils::CreateXML(device);
    logger.log("Generated XML: \n" + xmlString);  // Log the generated XML
}

// Function to print the parsed structure
void printStructure(const Pipeline &pipeline) {
    std::ostringstream oss;

    // Printing the Pipeline's basic information
    oss << "Printing Pipeline Data\n\nPipeline ID: " << pipeline.getPipelineID() << "\n";
    oss << "Request ID: " << pipeline.getRequestID() << "\n";
    oss << "Action: " << static_cast<int>(pipeline.getEAction()) << "\n";

    // Printing the MediaStreamDevice data (from the Pipeline)
    const MediaStreamDevice& device = pipeline.getMediaStreamDevice();

    oss << "\nDevice Name: " << device.sDeviceName << "\n";
    oss << "Input Source Type: " << static_cast<int>(device.stinputMediaData.esourceType) << "\n";
    oss << "Input Video Codec: " << static_cast<int>(device.stinputMediaData.stMediaCodec.evideocodec) << "\n";
    oss << "Input Audio Codec: " << static_cast<int>(device.stinputMediaData.stMediaCodec.eaudiocodec) << "\n";
    oss << "Input Audio Sample Rate: " << static_cast<int>(device.stinputMediaData.stMediaCodec.eaudioSampleRate) << "\n";
    oss << "Input Container Format: " << static_cast<int>(device.stinputMediaData.stFileSource.econtainerFormat) << "\n";
    oss << "Input Streaming Protocol: " << static_cast<int>(device.stinputMediaData.stNetworkStreaming.estreamingProtocol) << "\n";
    oss << "Input IP Address: " << device.stinputMediaData.stNetworkStreaming.sIpAddress << "\n";
    oss << "Input Port: " << device.stinputMediaData.stNetworkStreaming.iPort << "\n";
    oss << "Input Streaming Type: " << static_cast<int>(device.stinputMediaData.estreamingType) << "\n";

    oss << "Output Source Type: " << static_cast<int>(device.stoutputMediaData.esourceType) << "\n";
    oss << "Output Video Codec: " << static_cast<int>(device.stoutputMediaData.stMediaCodec.evideocodec) << "\n";
    oss << "Output Audio Codec: " << static_cast<int>(device.stoutputMediaData.stMediaCodec.eaudiocodec) << "\n";
    oss << "Output Audio Sample Rate: " << static_cast<int>(device.stoutputMediaData.stMediaCodec.eaudioSampleRate) << "\n";
    oss << "Output Container Format: " << static_cast<int>(device.stoutputMediaData.stFileSource.econtainerFormat) << "\n";
    oss << "Output Streaming Protocol: " << static_cast<int>(device.stoutputMediaData.stNetworkStreaming.estreamingProtocol) << "\n";
    oss << "Output IP Address: " << device.stoutputMediaData.stNetworkStreaming.sIpAddress << "\n";
    oss << "Output Port: " << device.stoutputMediaData.stNetworkStreaming.iPort << "\n";
    oss << "Output Streaming Type: " << static_cast<int>(device.stoutputMediaData.estreamingType) << "\n";
    
    oss << "------------------------------------------------------------------------------------------------------\n";
    // Log the structure information
    logger.log(oss.str());
}

// Parsing thread that takes generated strings and parses them
void parseGeneratedString() {
    while (keepRunning) {
        std::unique_lock<std::mutex> lock(parseGeneratedmtx);
        
        // Wait until there is something to process
        cvparseGenerate.wait(lock, []{ return !stringQueue.isEmpty(); });

        // Get the string from the queue
        std::string generatedString = stringQueue.dequeue();
        
        lock.unlock();  // Unlock the mutex before processing

        //create Pipeline info 
        Pipeline Pipelinedata;
        // Parse the string based on its format (JSON or XML)
        MediaStreamDevice parsedDevice;
        if (generatedString.substr(0, 1) == "{") {
            // Parse JSON
            JSONUtils::ParseJSON(generatedString, parsedDevice);

            //create Pipeline info 
            Pipelinedata.setPipelineID(RandomGenerator::getRandomValue());
            Pipelinedata.setRequestID(RandomGenerator::getRandomValue());
            Pipelinedata.setEAction(RandomGenerator::getRandomAction());
            Pipelinedata.setMediaStreamDevice(parsedDevice);

            int size = (sizeof(Pipelinedata)/(1024*1024));

            std::cout << "Size of Pipeline Data in MB : " << size << std::endl;

            logger.log("Parsed JSON");  // Log JSON parsing
        } else if (generatedString.substr(0, 1) == "<") {
            // Parse XML
            XMLUtils::ParseXML(generatedString, parsedDevice);

             //create Pipeline info 
            Pipelinedata.setPipelineID(RandomGenerator::getRandomValue());
            Pipelinedata.setRequestID(RandomGenerator::getRandomValue());
            Pipelinedata.setEAction(RandomGenerator::getRandomAction());
            Pipelinedata.setMediaStreamDevice(parsedDevice);

            logger.log("Parsed XML");  // Log XML parsing
        } else {
            logger.log("Unknown string format");  // Log unknown format
        }

        // Enqueue the parsed structure to parsedQueue
        parsedQueue.enqueue(Pipelinedata);
        cvprocessParsed.notify_all();
    }
}

// Processing thread that processes parsed structures
void processParsedStructure() {
    while (keepRunning) {
        std::unique_lock<std::mutex> lock(processParsedmtx);

        // Wait until there is something to process
        cvprocessParsed.wait(lock, []{ return !parsedQueue.isEmpty(); });

        // Get the parsed structure from the queue
        Pipeline Pipelinedata = parsedQueue.dequeue();
        
        lock.unlock();  // Unlock the mutex before processing

        // Here you can do any processing you'd like with the structure
        // For example, print the structure after processing
        printStructure(Pipelinedata);
    }
}

// Task thread that generates random data
void taskThreadFunction() {
    start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < RUN_DURATION) {
        // Sleep for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Generate a random MediaStreamDevice structure
        MediaStreamDevice generatedDevice = RandomGenerator::generateRandomMediaStreamDevice(rand() % arraySize);

        // Generate a random value between 0 and 1 to choose between XML or JSON
        int choice = rand() % 2;

        // Based on the random choice, create either XML or JSON
        std::string generatedString;
        if (choice == 0) {
            createXML(generatedDevice);
            generatedString = XMLUtils::CreateXML(generatedDevice);  // Store the string in generatedString
        } else {
            createJSON(generatedDevice);
            generatedString = JSONUtils::CreateJSON(generatedDevice);  // Store the string in generatedString
        }

        // Enqueue the generated string to stringQueue
        stringQueue.enqueue(generatedString);
        cvparseGenerate.notify_all();  // Notify all threads
    }
}

int main()
{
    gst_init(nullptr, nullptr);

    PipelineManager manager;

    Pipeline temp;
    temp.setPipelineID(1);
    MediaStreamDevice device;
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    temp.setMediaStreamDevice(device);

    manager.enqueuePipeline(temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    manager.startPipeline(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    Pipeline temp1;
    temp1.setPipelineID(2);
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    temp1.setMediaStreamDevice(device);

    manager.enqueuePipeline(temp1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    manager.startPipeline(2);

    std::this_thread::sleep_for(std::chrono::milliseconds(50000));
    manager.pausePipeline(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(50000));
    manager.startPipeline(2);
    // Keep running
    std::cout << "Press Enter to stop the pipeline..." << std::endl;
    std::cin.get();
    return 0;
}
