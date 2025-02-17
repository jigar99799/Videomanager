#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

// Logger class to handle logging to both console and file
class Logger {
public:
	enum class OutputType {
		Console,
		File
	};

	// Constructor, initializes the log file if the file output is chosen
	Logger(OutputType outputType = OutputType::Console, const std::string& filename = "output_log.txt")
		: outputType_(outputType), currentFileIndex_(0) {
		if (outputType == OutputType::File) {
			openNewLogFile(filename);
		}
	}

	// Log message function that writes either to console or file based on the output type
	void log(const std::string& message) {
		std::string timestamp = getCurrentTime();
		std::string logMessage = timestamp + " - " + message;

		if (outputType_ == OutputType::Console) {
			std::cout << logMessage << std::endl;
		}
		else if (outputType_ == OutputType::File && logFile_) {
			// Check if the file has exceeded the size limit
			if (logFile_.tellp() > static_cast<std::streamoff>(maxFileSize_)) {
				// Close the current log file and open a new one
				logFile_.close();
				openNewLogFile();
			}
			logFile_ << logMessage << std::endl;
		}
	}

	// Change output type (console or file) dynamically
	void setOutputType(OutputType newType, const std::string& filename = "") {
		if (outputType_ != newType) {
			outputType_ = newType;
			if (outputType_ == OutputType::File) {
				logFile_.close();
				openNewLogFile(filename);
			}
		}
	}

private:
	OutputType outputType_;
	std::ofstream logFile_;
	int currentFileIndex_;

	// Define the max file size as a constant (5 MB = 5 * 1024 * 1024 bytes)
	static constexpr size_t maxFileSize_ = 5 * 1024 * 1024;

	// Helper function to get current timestamp in a formatted string
	std::string getCurrentTime() {
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		std::tm tm = *std::localtime(&time);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		return oss.str();
	}

	// Open a new log file, with a unique name based on the index
	void openNewLogFile(const std::string& baseFilename = "output_log.txt") {
		std::string newFilename = baseFilename;
		if (currentFileIndex_ > 0) {
			// Add an index number to the filename to create a new file
			newFilename = baseFilename.substr(0, baseFilename.find_last_of('.')) 
				+ "_" + std::to_string(currentFileIndex_) 
				+ baseFilename.substr(baseFilename.find_last_of('.'));
		}
		logFile_.open(newFilename, std::ios::out | std::ios::app);
		if (!logFile_) {
			std::cerr << "Error opening file for logging." << std::endl;
		} else {
			// Increment the index for the next log file
			currentFileIndex_++;
		}
	}
};
