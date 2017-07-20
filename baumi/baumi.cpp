// baumi.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <iomanip>
#include <bgapi2_genicam.hpp>
#include <string.h>

// configure opencv: ok
// configure baumer: ok, if run outside Visual Studio

#define FRAMENUM_MAX 200
#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480

char filename[40];
using namespace cv;
using namespace std;
using namespace BGAPI2;

int main(int argc, char **argv)
{
	Mat *captured_frame, *frame;
	frame = new Mat(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);
	captured_frame= new Mat[FRAMENUM_MAX];
	unsigned long long int timestamp[FRAMENUM_MAX];

	BGAPI2::SystemList *systemList = NULL;
	BGAPI2::System * pSystem = NULL;
	BGAPI2::String sSystemID;

	BGAPI2::InterfaceList *interfaceList = NULL;
	BGAPI2::Interface * pInterface = NULL;
	BGAPI2::String sInterfaceID;

	BGAPI2::DeviceList *deviceList = NULL;
	BGAPI2::Device * pDevice = NULL;
	BGAPI2::String sDeviceID;

	BGAPI2::DataStreamList *datastreamList = NULL;
	BGAPI2::DataStream * pDataStream = NULL;
	BGAPI2::String sDataStreamID;

	BGAPI2::BufferList *bufferList = NULL;
	BGAPI2::Buffer * pBuffer = NULL;
	BGAPI2::String sBufferID;
	BGAPI2::String sExposureNodeName = "";

	int returncode = 0;

//list of systems
	try{
		systemList = SystemList::GetInstance();
		systemList->Refresh();
		std::cout << "Detected systems: "<< systemList->size() << std::endl;
		int i = 0;
		for (SystemList::iterator sysIterator = systemList->begin(); sysIterator != systemList->end(); sysIterator++){
			try{
				sysIterator->second->Open();
				std::cout << i<< ") "<< sysIterator->second->GetFileName() << std::endl;
				sSystemID = sysIterator->first;
				i++;
				try{
					interfaceList = sysIterator->second->GetInterfaces();
					//COUNT AVAILABLE INTERFACES
					interfaceList->Refresh(100); // timeout of 100 msec
					std::cout << "\tDetected interfaces: "<< interfaceList->size() << std::endl;

					//INTERFACE INFORMATION
					for (InterfaceList::iterator ifIterator = interfaceList->begin(); ifIterator != interfaceList->end(); ifIterator++)
					{
						std::cout << "\tInterface Name: "<< ifIterator->second->GetDisplayName() << std::endl;
					}
				}
				catch (BGAPI2::Exceptions::IException& ex)
				{
					returncode = 0 == returncode ? 1 : returncode;
					std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
					std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
					std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
				}

				//OPEN THE NEXT INTERFACE IN THE LIST
				try
				{
					for (InterfaceList::iterator ifIterator = interfaceList->begin(); ifIterator != interfaceList->end(); ifIterator++)
					{
						try
						{
							ifIterator->second->Open();
							//search for any camera is connetced to this interface
							deviceList = ifIterator->second->GetDevices();
							deviceList->Refresh(100);
							if (deviceList->size() == 0)
							{
								std::cout << "\t"<<deviceList->size() << "cameras found."	<< std::endl;
								ifIterator->second->Close();
							}
							else
							{
								sInterfaceID = ifIterator->first;
								if (ifIterator->second->GetTLType() == "U3V")
								{
								//	std::cout << "NodeListCount: "<< ifIterator->second->GetNodeList()->GetNodeCount() << std::endl; 
								}
								break;
							}
						}
						catch (BGAPI2::Exceptions::ResourceInUseException& ex)
						{
							returncode = 0 == returncode ? 1 : returncode;
							std::cout << "Interface "<< ifIterator->first << "already opened "<< std::endl;
							std::cout << "ResourceInUseException: "<< ex.GetErrorDescription() << std::endl;
						}
					}
				}
				catch (BGAPI2::Exceptions::IException& ex)
				{
					returncode = 0 == returncode ? 1 : returncode;
					std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
					std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
					std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
				}


				//if a camera is connected to the system interface then leave the system loop
				if (sInterfaceID != "")
				{
					break;
				}
			}
			catch (BGAPI2::Exceptions::ResourceInUseException& ex)
			{
				returncode = 0 == returncode ? 1 : returncode;
				std::cout << "System "<< sysIterator->first << "already opened "<< std::endl;
				std::cout << "ResourceInUseException: "<< ex.GetErrorDescription() << std::endl;
			}
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		returncode = 0 == returncode ? 1 : returncode;
		std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
		std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
		std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
	}

	if (sSystemID == "")
	{
		std::cout << "No System found "<< std::endl;
		std::cout << std::endl << "End"<< std::endl << "Input any number to close the program:";
		int endKey = 0;
		std::cin >> endKey;
		BGAPI2::SystemList::ReleaseInstance();
		return returncode;
	}
	else
	{
		pSystem = (*systemList)[sSystemID];
	}


	if (sInterfaceID == "")
	{
		std::cout << "No camera found "<< sInterfaceID << std::endl;
		std::cout << std::endl << "End"<< std::endl << "Input any number to close the program:";
		int endKey = 0;
		std::cin >> endKey;
		pSystem->Close();
		BGAPI2::SystemList::ReleaseInstance();
		return returncode;
	}
	else
	{
		pInterface = (*interfaceList)[sInterfaceID];
	}

	
	//OPEN DEVICE IN THE LIST
	try
	{
		deviceList = pInterface->GetDevices();
		deviceList->Refresh(100);
		for (DeviceList::iterator devIterator = deviceList->begin(); devIterator != deviceList->end(); devIterator++)
		{
			try
			{
				devIterator->second->Open();
				sDeviceID = devIterator->first;
				break;
			}
			catch (BGAPI2::Exceptions::ResourceInUseException& ex)
			{
				returncode = 0 == returncode ? 1 : returncode;
				std::cout << "Device "<< devIterator->first << "already opened "<< std::endl;
				std::cout << "ResourceInUseException: "<< ex.GetErrorDescription() << std::endl;
			}
			catch (BGAPI2::Exceptions::AccessDeniedException& ex)
			{
				returncode = 0 == returncode ? 1 : returncode;
				std::cout << "Device "<< devIterator->first << "already opened "<< std::endl;
				std::cout << "AccessDeniedException "<< ex.GetErrorDescription() << std::endl;
			}
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		returncode = 0 == returncode ? 1 : returncode;
		std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
		std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
		std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
	}

	if (sDeviceID == "")
	{
		std::cout << "No Device found "<< sDeviceID << std::endl;
		return(0);
	}
	else
	{
		pDevice = (*deviceList)[sDeviceID];
	}


	//OPEN THE FIRST DATASTREAM IN THE LIST
	try
	{
		datastreamList = pDevice->GetDataStreams();
		datastreamList->Refresh();
		for (DataStreamList::iterator dstIterator = datastreamList->begin(); dstIterator != datastreamList->end(); dstIterator++)
		{
			dstIterator->second->Open();
			sDataStreamID = dstIterator->first;
			if (dstIterator->second->GetTLType() == "GEV")
			{
				//std::cout << "StreamDriverModel: "<< dstIterator->second->GetNode("StreamDriverModel")->GetValue() << std::endl;
			}
			break;
		}
	}
	catch (BGAPI2::Exceptions::IException& ex)
	{
		returncode = 0 == returncode ? 1 : returncode;
		std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
		std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
		std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
	}

	if (sDataStreamID == "")
	{
		std::cout << "No DataStream found "<< sDataStreamID << std::endl;
		std::cout << std::endl << "End"<< std::endl << "Input any number to close the program:";
		int endKey = 0;
		std::cin >> endKey;
		pDevice->Close();
		pInterface->Close();
		pSystem->Close();
		BGAPI2::SystemList::ReleaseInstance();
		return returncode;
	}
	else
	{
		pDataStream = (*datastreamList)[sDataStreamID];
	}

try
{
	//BufferList
	bufferList = pDataStream->GetBufferList();

	// 4 buffers using internal buffer mode
	for (int i = 0; i<4; i++)
	{
		pBuffer = new BGAPI2::Buffer();
		bufferList->Add(pBuffer);
	}
	std::cout << "\tAnnounced buffers: "<< bufferList->GetAnnouncedCount() << "using "<< pBuffer->GetMemSize() * bufferList->GetAnnouncedCount() << "[bytes]"<< std::endl;
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}

try
{
	for (BufferList::iterator bufIterator = bufferList->begin(); bufIterator != bufferList->end(); bufIterator++)
	{
		bufIterator->second->QueueBuffer();
	}
	std::cout << "\tQueued buffers: "<< bufferList->GetQueuedCount() << std::endl;
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}
std::cout << ""<< std::endl;

//START DataStream acquisition
try
{
	pDataStream->StartAcquisitionContinuous();
	std::cout << "DataStream started "<< std::endl;
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}

//START CAMERA
// setup
pDevice->GetRemoteNode("ExposureTime")->SetDouble(100.00); //us
pDevice->GetRemoteNode("AcquisitionFrameRateEnable")->SetBool(true);
pDevice->GetRemoteNode("AcquisitionFrameRate")->SetDouble(1000.00); // fps
pDevice->GetRemoteNode("TriggerSoftware")->Execute();
cout << "Frame size: " << pDevice->GetRemoteNode("Width")->GetInt() << "x" << pDevice->GetRemoteNode("Height")->GetInt() << "px\n";

try
{
	std::cout << ""<< pDevice->GetModel() << "started "<< std::endl;
	pDevice->GetRemoteNode("AcquisitionStart")->Execute();
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}

//CAPTURE bunch of IMAGES
BGAPI2::Buffer * pBufferFilled = NULL;
try
{
	for (int framenum = 0; framenum < FRAMENUM_MAX; framenum++)
	{
		//WAIT FOR IMAGE
		pBufferFilled = pDataStream->GetFilledBuffer(50); //msec
		if (pBufferFilled == NULL)
		{
			std::cout << "Error: Buffer Timeout after 50 msec"<< std::endl;
		}
		else if (pBufferFilled->GetIsIncomplete() == true)
		{
			std::cout << "Error: Image is incomplete"<< std::endl;
			// queue buffer again
			pBufferFilled->QueueBuffer();
		}
		else
		{
			//std::cout << "Image "<< std::setw(5) << pBufferFilled->GetFrameID() << \
				"address "<< std::hex << pBufferFilled->GetMemPtr() << \
				"offset"<< std::dec << pBufferFilled->GetImagePresent() << \
				std::endl;
			memcpy(frame->data, pBufferFilled->GetMemPtr(), IMAGE_WIDTH*IMAGE_HEIGHT); // live frame
			captured_frame[framenum] = frame->clone(); // frames to save
			timestamp[framenum]= pBufferFilled->GetTimestamp();
			pBufferFilled->QueueBuffer();
		}
	}
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}
std::cout << ""<< std::endl;


//STOP CAMERA
std::cout << "CAMERA STOP"<< std::endl;
try
{
	if (pDevice->GetRemoteNodeList()->GetNodePresent("AcquisitionAbort"))
	{
		pDevice->GetRemoteNode("AcquisitionAbort")->Execute();
		std::cout << pDevice->GetModel() << "aborted "<< std::endl;
	}

	pDevice->GetRemoteNode("AcquisitionStop")->Execute();
	std::cout << pDevice->GetModel() << "stopped "<< std::endl;
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}

//STOP DataStream acquisition
try
{
	if (pDataStream->GetTLType() == "U3V")
	{
		//DataStream Statistic
		std::cout << "DataStream Statistics "<< std::endl;
		std::cout << "GoodFrames: "<< pDataStream->GetNodeList()->GetNode("GoodFrames")->GetInt() << std::endl;
		std::cout << "CorruptedFrames: "<< pDataStream->GetNodeList()->GetNode("CorruptedFrames")->GetInt() << std::endl;
		std::cout << "LostFrames: "<< pDataStream->GetNodeList()->GetNode("LostFrames")->GetInt() << std::endl;
		std::cout << std::endl;
	}

	pDataStream->StopAcquisition();
	bufferList->DiscardAllBuffers();
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}
std::cout << std::endl;


std::cout << "RELEASE"<< std::endl;

//Release buffers
try
{
	while (bufferList->size() > 0)
	{
		pBuffer = bufferList->begin()->second;
		bufferList->RevokeBuffer(pBuffer);
		delete pBuffer;
	}
	pDataStream->Close();
	pDevice->Close();
	pInterface->Close();
	pSystem->Close();
	BGAPI2::SystemList::ReleaseInstance();
}
catch (BGAPI2::Exceptions::IException& ex)
{
	returncode = 0 == returncode ? 1 : returncode;
	std::cout << "ExceptionType: "<< ex.GetType() << std::endl;
	std::cout << "ErrorDescription: "<< ex.GetErrorDescription() << std::endl;
	std::cout << "in function: "<< ex.GetFunctionName() << std::endl;
}

// saving frames
for (int framenum = 0; framenum<FRAMENUM_MAX; framenum++) {
	cout << framenum << ";"<< timestamp[framenum] << endl;
	sprintf_s(filename, "baum%04d.tif", framenum);
	//imwrite(filename, captured_frame[framenum]);
}

std::cout << "Input anything to close the program:";
int endKey = 0;
std::cin >> endKey;
return returncode;
}