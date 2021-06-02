// VRTRIXDataGloveTest.cpp : Demo project to show the usage of VRTRIX_IMU.dll
//

#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <conio.h>
#include <mutex>
#include <thread>
#include <Eigen/Dense>
#include "direct/VRTRIX_IMU.h" 
#include <windows.h>

#define RADTODEGREE 180.0f/M_PI

std::mutex mtx;
VRTRIX::IVRTRIXIMU* pRightHandDataGlove;
VRTRIX::IVRTRIXIMU* pLeftHandDataGlove;
bool bIsLHGloveConnected = false;
bool bIsRHGloveConnected = false;

// Helper function to return joint name as a string by its index.
std::string GetJointNamebyIndex(int index)
{
	switch (static_cast<VRTRIX::Joint>(index)) {
	case VRTRIX::Pinky_Intermediate: return "Pinky_Intermediate";
	case VRTRIX::Ring_Intermediate: return "Ring_Intermediate";
	case VRTRIX::Middle_Intermediate: return "Middle_Intermediate";
	case VRTRIX::Index_Intermediate: return "Index_Intermediate";
	case VRTRIX::Thumb_Distal: return "Thumb_Distal";
	case VRTRIX::Wrist_Joint: return "Wrist_Joint";
	case VRTRIX::Pinky_Proximal: return "Pinky_Proximal";
	case VRTRIX::Ring_Proximal: return "Ring_Proximal";
	case VRTRIX::Middle_Proximal: return "Middle_Proximal";
	case VRTRIX::Index_Proximal: return "Index_Proximal";
	case VRTRIX::Thumb_Intermediate: return "Thumb_Intermediate";
	case VRTRIX::Pinky_Distal: return "Pinky_Distal";
	case VRTRIX::Ring_Distal: return "Ring_Distal";
	case VRTRIX::Middle_Distal: return "Middle_Distal";
	case VRTRIX::Index_Distal: return "Index_Distal";
	case VRTRIX::Thumb_Proximal: return "Thumb_Proximal";
	default: return "Unknown";
	}
}
Eigen::Quaterniond VRTRIXQuat2EigenQuat(VRTRIX::VRTRIXQuaternion_t q) {
	return Eigen::Quaterniond(q.qw, q.qx, q.qy, q.qz);
}

//!  VRTRIX IMU event handler class implementation. 
/*!
     Implementation of IVRTRIXIMUEventHandler class that handles the IMU event including pose data receiving and other events.
*/
class CVRTRIXIMUEventHandler :public VRTRIX::IVRTRIXIMUEventHandler
{
	/** OnReceivedNewPose event call back function implement
	*
	* @param pose: Pose struct returned by the call back function
	* @param pUserParam: user defined parameter
	* @returns void
	*/
	void OnReceivedNewPose(VRTRIX::Pose pose, void* pUserParam) {
		std::unique_lock<std::mutex> lck(mtx); 
		VRTRIX::PortInfo * info = (VRTRIX::PortInfo *)pUserParam;
		switch (pose.type)
		{
		case(VRTRIX::Hand_Left): {
			if (info->index == 0) {
				//std::cout << "Left Hand Data Packet Received:" << std::endl;
				//std::cout << "	Battery (Left Hand): " << pose.battery << "%" << std::endl;
				//std::cout << "	Radio Strength (Left Hand): " << -pose.radioStrength << "dB" << std::endl;
				//for (int i = 0; i < (sizeof(pose.imuData) / sizeof(*pose.imuData)); i++) {
				//	std::cout << "	" << GetJointNamebyIndex(i) << ": " << pose.imuData[i];
				//}
				//std::cout << std::endl;
			}
		}
		break;
		case(VRTRIX::Hand_Right): {
			if (info->index == 0) {
				//std::cout << "Right Hand Data Packet Received:" << std::endl;
				//std::cout << "	Battery (Right Hand): " << pose.battery << "%" << std::endl;
				//std::cout << "	Radio Strength (Right Hand): " << -pose.radioStrength << "dB" << std::endl;
				//for (int i = 0; i < (sizeof(pose.imuData) / sizeof(*pose.imuData)); i++) {
				//	std::cout << "	" << GetJointNamebyIndex(i) << ": " << pose.imuData[i];
				//}
				//std::cout << std::endl;			
			}
		}
		break;
		default:
			break;
		}
	}

	/** OnReceivedNewEvent event call back function implement
	*
	* @param event: Event struct returned by the call back function
	* @param pUserParam: user defined parameter
	* @returns void
	*/
	void OnReceivedNewEvent(VRTRIX::HandEvent event, void* pUserParam) {
		switch (event.stat) {
		case(VRTRIX::HandStatus_Connected): {
			if (event.type == VRTRIX::Hand_Left) {
				bIsLHGloveConnected = true;
				std::cout << "Left Hand Glove Connected, data rate: "  << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				bIsRHGloveConnected = true;
				std::cout << "Right Hand Glove Connected, data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_InsufficientDataPacket): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand Glove insufficient data packet, data rate: " << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand Glove insufficient data packet, data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_Disconnected): {
			if (event.type == VRTRIX::Hand_Left) {
				bIsLHGloveConnected = false;
				std::cout << "Left Hand Glove Disconnected." << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				bIsRHGloveConnected = false;
				std::cout << "Right Hand Glove Disconnected." << std::endl;
			}
			break;
		}
		default: break;
		}
	}
};

/** Function for initializing data gloves & performing data streaming
*
* @param info: Data glove port info
* @param pDataGlove: Data glove object pointer, returned after it is initialized.
* @returns whether the initialization is successful.
*/
bool InitDataGloveStreaming(VRTRIX::PortInfo& portInfo, VRTRIX::IVRTRIXIMU* & pDataGlove) {
	if (portInfo.type == VRTRIX::Hand_Left) {
		std::cout << "**************************************" << std::endl;
		std::cout << "Left Hand Glove Initialization Start!" << std::endl;
		std::cout << "**************************************" << std::endl;
	}
	else if (portInfo.type == VRTRIX::Hand_Right) {
		std::cout << "**************************************" << std::endl;
		std::cout << "Right Hand Glove Initialization Start!" << std::endl;
		std::cout << "**************************************" << std::endl;
	}
	VRTRIX::EInitError eInitError;
	VRTRIX::EIMUError eIMUError;
	//Initialize event handler.
	VRTRIX::IVRTRIXIMUEventHandler* pEventHandler = new CVRTRIXIMUEventHandler();
	//Initialize data glove.
	pDataGlove = InitDataGlove(eInitError, VRTRIX::InitMode_Normal, VRTRIX::PRO);
	if (eInitError != VRTRIX::InitError_None) {
		std::cout << "Error: " << eInitError << std::endl;
		return false;
	}
	//Register event call back and perform events handling/pose updating.
	pDataGlove->RegisterIMUDataCallback(pEventHandler, &portInfo);

	//Start data streaming.
	pDataGlove->StartDataStreaming(eIMUError, portInfo);
	if (eIMUError == VRTRIX::IMUError_ConnectionBusy) {
		if (portInfo.type == VRTRIX::Hand_Left) std::cout << "LEFT HAND PORT OPEN FAILED! " << std::endl;
		else std::cout << "RIGHT HAND PORT OPEN FAILED! " << std::endl;
		return false;
	}
	std::cout << std::endl;
	return true;
}

int main()
{
	VRTRIX::EIMUError eIMUError;
	VRTRIX::PortInfo gloveLHPort;
	VRTRIX::PortInfo gloveRHPort;
	gloveLHPort.type = VRTRIX::Hand_Left;
	gloveRHPort.type = VRTRIX::Hand_Right;

	InitDataGloveStreaming(gloveLHPort, pLeftHandDataGlove);
	InitDataGloveStreaming(gloveRHPort, pRightHandDataGlove);

	//Advanced mode flag
	bool advancedMode = false;

	while (1) {
		int key = _getch();
		//If Esc is pressed, close the connection and exit the program.
		if (key == 27) {
			std::cout << "ClosePort" << std::endl;
			pLeftHandDataGlove->ClosePort(eIMUError);
			UnInit(pLeftHandDataGlove);

			pRightHandDataGlove->ClosePort(eIMUError);
			UnInit(pRightHandDataGlove);
			break;
		}

		//If a is pressed, close the connection and restart.
		if (key == 'a') {
			std::cout << "ClosePort" << std::endl;
			pLeftHandDataGlove->ClosePort(eIMUError);
			UnInit(pLeftHandDataGlove);

			pRightHandDataGlove->ClosePort(eIMUError);
			UnInit(pRightHandDataGlove);

			std::cout << "Start data streaming again" << std::endl;
			InitDataGloveStreaming(gloveLHPort, pLeftHandDataGlove);
			InitDataGloveStreaming(gloveRHPort, pRightHandDataGlove);
		}

		//If d is pressed, align both finger orientation to the hand (what we called "software alignment")
		// Note: Only use this when advanced mode is set to true.
		if (key == 'd') {
			pLeftHandDataGlove->TPoseCalibration(eIMUError);
			pRightHandDataGlove->TPoseCalibration(eIMUError);
		}

		//If 'f' is pressed, switch to advanced mode that yaw of fingers will NOT be locked.
		// Note: Only use this when magnetic field is stable. 
		if (key == 'f') {
			advancedMode = !advancedMode;
			advancedMode ? std::cout << "Turn on advanced mode!" << std::endl : std::cout << "Turn off advanced mode!" << std::endl;
			pLeftHandDataGlove->SwitchToAdvancedMode(eIMUError, advancedMode);
			pRightHandDataGlove->SwitchToAdvancedMode(eIMUError, advancedMode);
		}
	}

	return 0;
}

