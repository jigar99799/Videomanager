#include <thread>
#include <condition_variable>
#include <atomic>
#include <random>
#include <string>
#include <chrono>
#include <iostream>

#include "Pipeline.h"
#include "MxPipelineManager.h"
#include "MediaStreamDevice.h"
#include "RandomGenerator.h"


// Helper function to create a test MediaStreamDevice
MediaStreamDevice createTestDevice(const std::string& rtspUrl, eSourceType sourceType) 
{
    MediaStreamDevice device;
    
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stinputMediaData.stMediaCodec.type = CodecType::H264;
    device.stinputMediaData.stMediaCodec.bitrate = 20000;
    device.stinputMediaData.stNetworkStreaming.sIpAddress = "192.168.111.11";
    device.stinputMediaData.stNetworkStreaming.iPort = 1050;


    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    device.stoutputMediaData.stMediaCodec.type = CodecType::H264;


    return device;
}

// Helper function to print pipeline status
void printPipelineStatus(const PipelineManager& manager) {
    auto activePipelines = manager.getActivePipelines();
    std::cout << "\nActive Pipelines: " << activePipelines.size() << std::endl;
    for (const auto& id : activePipelines) {
        std::cout << "Pipeline ID: " << id << " is running" << std::endl;
    }
    std::cout << "Queue Size: " << manager.getQueueSize() << std::endl;
}

int main()
{
    try 
    {
        // Create pipeline manager
        PipelineManager manager;
        
        // Initialize logger
        if (!manager.initializeLogger("debug_config.json")) 
        {
            std::cerr << "Warning: Failed to initialize logger, continuing with default logging" << std::endl;
        }
        
        
        // Create first pipeline
        auto device1 = createTestDevice("rtsp://admin:admin@192.168.111.150/unicaststream/1", eSourceType::SOURCE_TYPE_NETWORK);
       
        auto id1 = manager.createPipeline(device1);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        manager.sendPipelineRequest(id1,device1,PipelineOperation::Run);
       // manager.startPipeline(id1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        manager.sendPipelineRequest(id1,device1,PipelineOperation::Pause);

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        manager.sendPipelineRequest(id1,device1,PipelineOperation::Resume);



        //// Create second pipeline
        //auto device2 = createTestDevice("rtsp://admin:admin@192.168.111.11/unicaststream/1", eSourceType::SOURCE_TYPE_NETWORK);
        //auto id2 = manager.createPipeline(device2);
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        //manager.startPipeline(id2);
        //
        //printPipelineStatus(manager);
        //
        //// Test Case 2: Pipeline Control Operations
        //std::cout << "\n=== Test Case 2: Pipeline Control Operations ===" << std::endl;
        //
        //// Pause pipeline 1
        //std::cout << "Pausing pipeline " << id1 << std::endl;
        //manager.pausePipeline(id1);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        //
        //// Resume pipeline 1
        //std::cout << "Resuming pipeline " << id1 << std::endl;
        //manager.resumePipeline(id1);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        //
        //// Test Case 3: Pipeline Updates
        //std::cout << "\n=== Test Case 3: Pipeline Updates ===" << std::endl;
        //
        //// Update pipeline 2 configuration
        //auto updatedDevice2 = device2;
        //Resolution res = updatedDevice2.resolution();
        //res.width = 1280;
        //res.height = 720;
        //updatedDevice2.setResolution(res);
        //std::cout << "Updating pipeline " << id2 << " configuration" << std::endl;
        //manager.updatePipeline(id2, updatedDevice2);
        //
        //// Test Case 4: Pipeline Termination
        //std::cout << "\n=== Test Case 4: Pipeline Termination ===" << std::endl;
        //
        //// Stop pipeline 1
        //std::cout << "Stopping pipeline " << id1 << std::endl;
        //manager.stopPipeline(id1);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        //
        //// Terminate pipeline 2
        //std::cout << "Terminating pipeline " << id2 << std::endl;
        //manager.terminatePipeline(id2);
        //
        //printPipelineStatus(manager);
        //
        //// Test Case 5: Error Handling
        //std::cout << "\n=== Test Case 5: Error Handling ===" << std::endl;
        //
        //// Try to start non-existent pipeline
        //std::cout << "Attempting to start non-existent pipeline..." << std::endl;
        //if (!manager.startPipeline(999)) {
        //    std::cout << "Successfully handled invalid pipeline ID" << std::endl;
        //}
        //
        //// Test Case 6: Stress Test
        //std::cout << "\n=== Test Case 6: Stress Test ===" << std::endl;
        //
        //// Create multiple pipelines rapidly
        //std::vector<PipelineID> pipelineIds;
        //const int numPipelines = 10;
        //for (int i = 0; i < numPipelines; ++i) {
        //    auto stressDevice = createTestDevice(
        //        "rtsp://stress" + std::to_string(i) + ".example.com/stream1",
        //        eSourceType::SOURCE_TYPE_NETWORK
        //    );
        //    auto id = manager.createPipeline(stressDevice);
        //    pipelineIds.push_back(id);
        //    manager.startPipeline(id);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //}
        //
        //printPipelineStatus(manager);
        //
        //// Cleanup stress test pipelines
        //for (auto id : pipelineIds) {
        //    manager.terminatePipeline(id);
        //}
        //
        std::cout << "\n=== All tests completed ===" << std::endl;
        
        // Wait for user input before exiting
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
