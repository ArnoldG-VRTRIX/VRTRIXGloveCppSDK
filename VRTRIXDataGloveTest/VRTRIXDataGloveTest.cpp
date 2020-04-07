// VRTRIXDataGloveTest.cpp : Demo project to show the usage of VRTRIX_IMU.dll
//

#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <conio.h>
#include <mutex>
#include <thread>
#include <Eigen/Dense>
#include "VRTRIX_IMU.h"

#define RADTODEGREE 180.0f/M_PI

std::mutex mtx;
VRTRIX::VRTRIXQuaternion_t rhData[VRTRIX::Joint_MAX];
bool bIsQuit = false;
bool startDataCount = false;
bool bIsLHGloveConnected = false;
bool bIsRHGloveConnected = false;
int dataCount = 0;

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

double angleBetweenVectors(Eigen::Vector3d a, Eigen::Vector3d b) {
	//return std::atan2(a.cross(b).norm(), a.dot(b)) * RADTODEGREE;
	return std::acos(a.dot(b) / (a.norm()*b.norm()))*RADTODEGREE;
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
		switch (pose.type)
		{
		case(VRTRIX::Hand_Left): {
			//std::cout << "Left Hand Data Packet Received:" << std::endl;
			//std::cout << "	Battery (Left Hand): " << pose.battery << "%" << std::endl;
			//std::cout << "	Radio Strength (Left Hand): " << pose.radioStrength << std::endl;
			//for (int i = 0; i < (sizeof(pose.imuData) / sizeof(*pose.imuData)); i++) {
			//	std::cout << "	" << GetJointNamebyIndex(i) << ": " << pose.imuData[i];
			//}
			//std::cout << std::endl;
		}
		break;
		case(VRTRIX::Hand_Right): {
			if (startDataCount) {
				dataCount++;
			}
			for (int i = 0; i < VRTRIX::Joint_MAX; ++i) {
				rhData[i] = pose.imuData[i];
			}
			//std::cout << "Right Hand Data Packet Received:" << std::endl;
			//std::cout << "	Battery (Right Hand): " << pose.battery << "%" << std::endl;
			//std::cout << "	Radio Strength (Right Hand): " << pose.radioStrength << std::endl;
			//for (int i = 0; i < (sizeof(pose.imuData) / sizeof(*pose.imuData)); i++) {
			//	std::cout << "	" << GetJointNamebyIndex(i) << ": " << pose.imuData[i];
			//}
			//std::cout << std::endl;
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
		case(VRTRIX::HandStatus_LowBattery): {
			std::cout << "Low Battery!" << std::endl;
			break;
		}
		case(VRTRIX::HandStatus_BatteryFull): {
			std::cout << "Battery Full!" << std::endl;
			break;
		}
		case(VRTRIX::HandStatus_Paired): {
			std::cout << "Data Glove Paired!" << std::endl;
			break;
		}
		case(VRTRIX::HandStatus_MagAbnormal): {
			std::cout << "Magnetic Abnormal Detected!" << std::endl;
			break;
		}
		case(VRTRIX::HandStatus_Connected): {
			if (event.type == VRTRIX::Hand_Left) {
				bIsLHGloveConnected = true;
				std::cout << "Left Hand Glove is connected, data streaming started!" << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				bIsRHGloveConnected = true;
				std::cout << "Right Hand Glove is connected, data streaming started!" << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_Disconnected): {
			if (event.type == VRTRIX::Hand_Left) {
				bIsLHGloveConnected = false;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				bIsRHGloveConnected = false;
			}
			break;
		}
		default: break;
		}
	}

};

/** Thread function for initializing data gloves & performing data streaming
*
* @param type: Hand type enum
* @param pDataGlove: Data glove object pointer, returned after it is initialized.
* @returns void
*/
void InitDataGloveStreaming(VRTRIX::HandType type, VRTRIX::IVRTRIXIMU* &pDataGlove) {

	VRTRIX::EInitError eInitError;
	VRTRIX::EIMUError eIMUError;
	//Initialize event handler.
	VRTRIX::IVRTRIXIMUEventHandler* pEventHandler = new CVRTRIXIMUEventHandler();
	//Initialize data glove.
	pDataGlove = InitDataGlove(eInitError);
	if (eInitError != VRTRIX::InitError_None) {
		std::cout << "Error: " << eInitError << std::endl;
		return;
	}
	//Register event call back and perform events handling/pose updating.
	pDataGlove->RegisterIMUDataCallback(pEventHandler);
	//Prepare PortInfo struct and open the data streaming serial port of glove.
	VRTRIX::PortInfo portInfo;
	portInfo.type= type;
	pDataGlove->OpenPort(eIMUError, portInfo);

	if (eIMUError == VRTRIX::IMUError_None) {
		//Print out full port information
		std::cout << "PORT DESCIPTION: " << portInfo.description << std::endl;
		std::cout << "PORT HARDWARE ID: " << portInfo.hardware_id << std::endl;
		std::cout << "PORT INSTANCE ID: " << portInfo.instance_id << std::endl;
		std::cout << "PORT NAME: " << portInfo.port << std::endl;
		std::cout << "PORT BAUD RATE: " << portInfo.baud_rate << std::endl;
		//Start data streaming.
		pDataGlove->StartDataStreaming(eIMUError, portInfo);
		if (eIMUError == VRTRIX::IMUError_ConnectionBusy) {
			if(type == VRTRIX::Hand_Left) std::cout << "LEFT HAND PORT OPEN FAILED! " << std::endl;
			else std::cout << "RIGHT HAND PORT OPEN FAILED! " << std::endl;
		}
	}
	else {
		pDataGlove->ClosePort(eIMUError);
		UnInit(pDataGlove);
	}
}

int main()
{
	//Start right hand data streaming
	VRTRIX::EIMUError eIMUError;

	VRTRIX::IVRTRIXIMU* pRightHandDataGlove;
	std::cout << "**************************************" << std::endl;
	std::cout << "Right Hand Glove Initialization Start!" << std::endl;
	std::cout << "**************************************" << std::endl;
	InitDataGloveStreaming(VRTRIX::Hand_Right, pRightHandDataGlove);
	std::cout << std::endl;

	////Start left hand data streaming
	VRTRIX::IVRTRIXIMU* pLeftHandDataGlove;
	std::cout << "**************************************" << std::endl;
	std::cout << "Left Hand Glove Initialization Start!" << std::endl;
	std::cout << "**************************************" << std::endl;
	InitDataGloveStreaming(VRTRIX::Hand_Left, pLeftHandDataGlove);
	std::cout << std::endl;

	//Advanced mode flag
	bool advancedMode = false;

	while (1) {
		int key = _getch();
		//If Esc is pressed, close the connection and exit the program.
		if (key == 27) {
			bIsQuit = true;
			std::cout << "ClosePort" << std::endl;
			if (pRightHandDataGlove != nullptr) {
				pRightHandDataGlove->ClosePort(eIMUError);
				UnInit(pRightHandDataGlove);
			}
			if (pLeftHandDataGlove != nullptr) {
				pLeftHandDataGlove->ClosePort(eIMUError);
				UnInit(pLeftHandDataGlove);
			}
			break;
		}
		//If s is pressed, close the connection and restart.
		if (key == 115) {
			std::cout << "ClosePort" << std::endl;
			if (pRightHandDataGlove != nullptr) {
				pRightHandDataGlove->ClosePort(eIMUError);
				UnInit(pRightHandDataGlove);
				bIsRHGloveConnected = false;
			}
			if (pLeftHandDataGlove != nullptr) {
				pLeftHandDataGlove->ClosePort(eIMUError);
				UnInit(pLeftHandDataGlove);
				bIsLHGloveConnected = false;
			}
			std::cout << "Start data streaming again" << std::endl;
			InitDataGloveStreaming(VRTRIX::Hand_Right, pRightHandDataGlove);
			InitDataGloveStreaming(VRTRIX::Hand_Left, pLeftHandDataGlove);
		}
		//If v is pressed, trigger a 1s haptic pulse for both hands.
		if (key == 118) {
			pRightHandDataGlove->VibratePeriod(eIMUError, 1000);
			pLeftHandDataGlove->VibratePeriod(eIMUError, 1000);
		}
		//If r is pressed, align both finger orientation to the hand (what we called "software alignment")
		// Note: Only use this when advanced mode is set to true.
		if (key == 114) {
			pRightHandDataGlove->SoftwareAlign(eIMUError);
			pLeftHandDataGlove->SoftwareAlign(eIMUError);
		}
		//If c is pressed, save current background calibration file to the hardware ROM (what we called "software alignment")
		// Note: All data gloves have performed IN-FACTORY hardware calibration, no need to do it again unless the environment 
		//       magnetic field has been changed dramatically. 
		if (key == 99) {
			pRightHandDataGlove->HardwareCalibrate(eIMUError);
			pLeftHandDataGlove->HardwareCalibrate(eIMUError);
		}
		//If a is pressed, switch to advanced mode that yaw of fingers will NOT be locked.
		// Note: Only use this when magnetic field is stable. 
		if (key == 97) {
			advancedMode = !advancedMode;
			advancedMode ? std::cout << "Turn on advanced mode!" << std::endl : std::cout << "Turn off advanced mode!" << std::endl;
			pRightHandDataGlove->SwitchToAdvancedMode(eIMUError, advancedMode);
			pLeftHandDataGlove->SwitchToAdvancedMode(eIMUError, advancedMode);
		}
		//If p is pressed, pairing left hand started.
		if (key == 112) {
			pLeftHandDataGlove->ClosePort(eIMUError);
			std::cout << "Close port error: " << eIMUError << std::endl;
			UnInit(pLeftHandDataGlove);

			VRTRIX::EInitError eInitError;
			//Initialize event handler.
			VRTRIX::IVRTRIXIMUEventHandler* pEventHandler = new CVRTRIXIMUEventHandler();
			//Initialize data glove.
			pLeftHandDataGlove = InitDataGlove(eInitError);
			if (eInitError != VRTRIX::InitError_None) {
				std::cout << "Error: " << eInitError << std::endl;
				continue;
			}
			//Register event call back and perform events handling/pose updating.
			pLeftHandDataGlove->RegisterIMUDataCallback(pEventHandler);
			//Prepare PortInfo struct and open the data streaming serial port of glove.
			//Note: the default baud_rate is 1000000 and should NOT be changed.
			VRTRIX::PortInfo portInfo;
			portInfo.baud_rate = 1000000;
			portInfo.type = VRTRIX::Hand_Left;
			pLeftHandDataGlove->OpenPort(eIMUError, portInfo);

			//Start to pair the left hand.
			pLeftHandDataGlove->RequestToPair(80, portInfo, eIMUError);
		}

		//If e is pressed, switch to rssi scanning mode.
		if (key == 101) {
			pLeftHandDataGlove->RequestToRSSIScan(eIMUError);
			std::cout << "RequestToRSSIScan error: " << eIMUError << std::endl;
		}
		//If g is pressed, com port setting is performed
		//*Note: This function need to be called when running on a new computer
		if (key == 103) {

			VRTRIX::PortInfo portInfo;
			portInfo.baud_rate = 1000000;
			portInfo.type = VRTRIX::Hand_Left;
			pLeftHandDataGlove->OpenPort(eIMUError, portInfo);
			pLeftHandDataGlove->ComPortLatencySetting(portInfo, eIMUError);
		}

		//If f is pressed, print out left hand finger angle
		if (key == 102) {
			double pinkyAngle = pLeftHandDataGlove->GetFingerBendAngle(VRTRIX::Pinky_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}
			double ringAngle = pLeftHandDataGlove->GetFingerBendAngle(VRTRIX::Ring_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}
			double middleAngle = pLeftHandDataGlove->GetFingerBendAngle(VRTRIX::Middle_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}
			double indexAngle = pLeftHandDataGlove->GetFingerBendAngle(VRTRIX::Index_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}
			double thumbAngle = pLeftHandDataGlove->GetFingerBendAngle(VRTRIX::Thumb_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}

			std::cout << "Pinky Bend Angle: " << pinkyAngle << ", ";
			std::cout << "Ring Bend Angle: " << ringAngle << ", ";
			std::cout << "Middle Bend Angle: " << middleAngle << ", ";
			std::cout << "Index Bend Angle: " << indexAngle << ", ";
			std::cout << "Thumb Bend Angle: " << thumbAngle << std::endl;
		}

		//If y is pressed, print out left hand index finger & middle finger yaw angle
		if (key == 121) {
			double index_middle = pLeftHandDataGlove->GetFingerYawAngle(VRTRIX::Middle_Intermediate, VRTRIX::Index_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}

			double middle_ring = pLeftHandDataGlove->GetFingerYawAngle(VRTRIX::Ring_Intermediate, VRTRIX::Middle_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}

			double ring_pinky = pLeftHandDataGlove->GetFingerYawAngle(VRTRIX::Pinky_Intermediate, VRTRIX::Ring_Intermediate, eIMUError);
			if (eIMUError != VRTRIX::IMUError_None) {
				std::cout << "IMU error: " << eIMUError << std::endl;
				continue;
			}

			std::cout << "Index_Middle Spacing: " << index_middle << " Middle_Ring Spacing: " << middle_ring << " Ring_Pinky Spacing: " << ring_pinky << std::endl;
		}
		
		//If d is pressed, print out glove data in next second at 60fps
		if (key == 100) {
			std::chrono::steady_clock::time_point loopStart = std::chrono::steady_clock::now();
			int index = 1;
			startDataCount = true;
			while (true) {
				if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - loopStart).count() >= 16.67) {
					loopStart = std::chrono::steady_clock::now();
					std::cout << "data index: " << index << std::endl;
					std::cout << "	" << GetJointNamebyIndex(0) << ": " << rhData[0];
					if (index >= 60) {
						std::cout << "total data received: " << dataCount << std::endl;
						startDataCount = false;
						dataCount = 0;
						break;
					}
					index++;
				}
			}
		}

		//If SPACE is pressed, print out current wrist pitch angle.
		if (key == 32) {
			Eigen::Quaterniond wristQuat = VRTRIXQuat2EigenQuat(rhData[VRTRIX::Wrist_Joint]);
			Eigen::Vector3d xAxisRotated = wristQuat * Eigen::Vector3d::UnitX();
			Eigen::Vector3d yAxisRotated = wristQuat * Eigen::Vector3d::UnitY();
			//If yAxis towards to sky, then the pitch angle is sharp.
			//Otherwise, if yAxis towards to ground, then the pitch angle is obtuse.
			Eigen::Vector3d xAxisRotatedProjected = (yAxisRotated.y() > 0) ? Eigen::Vector3d(xAxisRotated.x(), 0, xAxisRotated.z())
																		 : Eigen::Vector3d(-xAxisRotated.x(), 0, -xAxisRotated.z());
			double pitch = angleBetweenVectors(xAxisRotated, xAxisRotatedProjected);
			//If xAxis towards to sky, then pitch is minus.
			//Otherwise, xAxis towards to ground, then pitch is plus.
			if (xAxisRotated.y() > 0) pitch = -pitch;
			std::cout << "Right hand wrist pitch angle: " << pitch << std::endl;
		}
	}

	return 0;
}

