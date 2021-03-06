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
std::vector<VRTRIX::IVRTRIXIMU*> pRightHandDataGloves;
std::vector<VRTRIX::IVRTRIXIMU*> pLeftHandDataGloves;
VRTRIX::VRTRIXQuaternion_t rhData[VRTRIX::Joint_MAX];
VRTRIX::VRTRIXQuaternion_t lhData[VRTRIX::Joint_MAX];
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
		VRTRIX::PortInfo * info = (VRTRIX::PortInfo *)pUserParam;
		switch (pose.type)
		{
		case(VRTRIX::Hand_Left): {
			if (info->index == 0) {
				for (int i = 0; i < VRTRIX::Joint_MAX; ++i) {
					lhData[i] = pose.imuData[i];
				}
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
				for (int i = 0; i < VRTRIX::Joint_MAX; ++i) {
					rhData[i] = pose.imuData[i];
				}
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
				std::cout << "Left Hand Glove is connected at channel: " << event.channel << ", data rate: "  << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				bIsRHGloveConnected = true;
				std::cout << "Right Hand Glove is connected at channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_InsufficientDataPacket): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand Glove insufficient data packet at channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand Glove insufficient data packet at channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_Disconnected): {
			if (event.type == VRTRIX::Hand_Left) {
				bIsLHGloveConnected = false;
				std::cout << "Left Hand Glove is disconnected from channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				bIsRHGloveConnected = false;
				std::cout << "Right Hand Glove is disconnected from channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_ChannelHopping): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand Glove channel hopping from channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand Glove channel hopping from channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_NewChannelSelected): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand Glove select new channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand Glove select new channel: " << event.channel << ", data rate: " << event.dataRate << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_SetRadioLimit): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand Glove set radio limit: " << event.lowerBound << " - " << event.upperBound << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand Glove set radio limit: " << event.lowerBound << " - " << event.upperBound << std::endl;
			}
			break;
		}				
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
		case(VRTRIX::HandStatus_UpdateDongleFirmwarePrepared): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand dongle update firmware prepared!" << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand dongle update firmware prepared!" << std::endl;
			}
			break;
		}
		case(VRTRIX::HandStatus_UpdateGloveFirmwarePrepared): {
			if (event.type == VRTRIX::Hand_Left) {
				std::cout << "Left Hand glove update firmware prepared!" << std::endl;
			}
			else if (event.type == VRTRIX::Hand_Right) {
				std::cout << "Right Hand glove update firmware prepared!" << std::endl;
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

	//Print out full port information
	std::cout << "PORT INDEX: " << portInfo.index << std::endl;
	std::cout << "PORT DESCIPTION: " << portInfo.description << std::endl;
	std::cout << "PORT HARDWARE ID: " << portInfo.hardware_id << std::endl;
	std::cout << "PORT INSTANCE ID: " << portInfo.instance_id << std::endl;
	std::cout << "PORT NAME: " << portInfo.port << std::endl;
	std::cout << "PORT BAUD RATE: " << portInfo.baud_rate << std::endl;

	//Set radio channel limit according to data glove index before start data streaming if needed. (this step is optional)
	pDataGlove->SetRadioChannelLimit(eIMUError, 99 - portInfo.index * 16, 85 - portInfo.index * 16);
	if (eIMUError == VRTRIX::IMUError_DataNotValid) std::cout << "IMUError_DataNotValid!" << std::endl;

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
	//Get all data gloves connected to system
	VRTRIX::PortInfo glovePorts[MAX_GLOVE_SUPPORTED];
	int gloveNum = GetConnectedGlovePortInfo(eIMUError, glovePorts);

	//Iterate glove ports and start data glove initialization
	for (int i = 0; i < gloveNum; ++i) {
		VRTRIX::IVRTRIXIMU* pDataGlove;
		if (glovePorts[i].type == VRTRIX::Hand_Left) {
			if (InitDataGloveStreaming(glovePorts[i], pDataGlove)) pLeftHandDataGloves.push_back(pDataGlove);
		}
		else if (glovePorts[i].type == VRTRIX::Hand_Right) {
			if (InitDataGloveStreaming(glovePorts[i], pDataGlove)) pRightHandDataGloves.push_back(pDataGlove);
		}
	}

	if (gloveNum == 0) return 0;

	//Advanced mode flag
	bool advancedMode = false;

	while (1) {
		int key = _getch();
		//If Esc is pressed, close the connection and exit the program.
		if (key == 27) {
			std::cout << "ClosePort" << std::endl;
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->RequestToRSSIScan(eIMUError);
				pDataGlove->ClosePort(eIMUError);
				UnInit(pDataGlove);
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->RequestToRSSIScan(eIMUError);
				pDataGlove->ClosePort(eIMUError);
				UnInit(pDataGlove);
			}
			break;
		}
		//If a is pressed, close the connection and restart.
		if (key == 'a') {
			std::cout << "ClosePort" << std::endl;
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->RequestToRSSIScan(eIMUError);
				pDataGlove->ClosePort(eIMUError);
				UnInit(pDataGlove);
				bIsRHGloveConnected = false;
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->RequestToRSSIScan(eIMUError);
				pDataGlove->ClosePort(eIMUError);
				UnInit(pDataGlove);
				bIsLHGloveConnected = false;
			}
			pRightHandDataGloves.clear();
			pLeftHandDataGloves.clear();
			std::cout << "Start data streaming again" << std::endl;
			for (int i = 0; i < gloveNum; ++i) {
				VRTRIX::IVRTRIXIMU* pDataGlove;
				if (glovePorts[i].type == VRTRIX::Hand_Left) {
					if (InitDataGloveStreaming(glovePorts[i], pDataGlove)) pLeftHandDataGloves.push_back(pDataGlove);
				}
				else if (glovePorts[i].type == VRTRIX::Hand_Right) {
					if (InitDataGloveStreaming(glovePorts[i], pDataGlove)) pRightHandDataGloves.push_back(pDataGlove);
				}
			}
		}
		//If b is pressed, trigger a 1s haptic pulse for both hands.
		if (key == 'b') {
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->VibratePeriod(eIMUError, 1000);
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->VibratePeriod(eIMUError, 1000);
			}
		}
		//If c is pressed, toggle vibration;
		if (key == 'c') {
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->ToggleVibration(eIMUError);
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->ToggleVibration(eIMUError);
			}
		}
		//If d is pressed, align both finger orientation to the hand (what we called "software alignment")
		// Note: Only use this when advanced mode is set to true.
		if (key == 'd') {
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->TPoseCalibration(eIMUError);
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->TPoseCalibration(eIMUError);
			}
		}
		//If e is pressed, save current background calibration file to the hardware ROM (what we called "software alignment")
		// Note: All data gloves have performed IN-FACTORY hardware calibration, no need to do it again unless the environment 
		//       magnetic field has been changed dramatically. 
		if (key == 'e') {
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->HardwareCalibrate(eIMUError);
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->HardwareCalibrate(eIMUError);
			}
		}
		//If 'f' is pressed, switch to advanced mode that yaw of fingers will NOT be locked.
		// Note: Only use this when magnetic field is stable. 
		if (key == 'f') {
			advancedMode = !advancedMode;
			advancedMode ? std::cout << "Turn on advanced mode!" << std::endl : std::cout << "Turn off advanced mode!" << std::endl;
			for (auto pDataGlove : pRightHandDataGloves) {
				pDataGlove->SwitchToAdvancedMode(eIMUError, advancedMode);
			}
			for (auto pDataGlove : pLeftHandDataGloves) {
				pDataGlove->SwitchToAdvancedMode(eIMUError, advancedMode);
			}
		}
		//If g is pressed, pairing left hand started.
		if (key == 'g') {
			//Start to pair the left hand.
			if (pLeftHandDataGloves.size() >= 1) {
				pLeftHandDataGloves[0]->RequestToPair(80, eIMUError);
			}
		}

		//If h is pressed, switch to rssi scanning mode.
		if (key == 'h') {
			//Start to pair the left hand.
			if (pLeftHandDataGloves.size() >= 1) {
				pLeftHandDataGloves[0]->RequestToRSSIScan(eIMUError);
			}
			std::cout << "RequestToRSSIScan error: " << eIMUError << std::endl;
		}

		//If i is pressed, print out left hand finger angle
		if (key == 'i') {
			if (pLeftHandDataGloves.size() >= 1) {
				double pinkyAngle = pLeftHandDataGloves[0]->GetFingerBendAngle(VRTRIX::Pinky_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}
				double ringAngle = pLeftHandDataGloves[0]->GetFingerBendAngle(VRTRIX::Ring_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}
				double middleAngle = pLeftHandDataGloves[0]->GetFingerBendAngle(VRTRIX::Middle_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}
				double indexAngle = pLeftHandDataGloves[0]->GetFingerBendAngle(VRTRIX::Index_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}
				double thumbAngle = pLeftHandDataGloves[0]->GetFingerBendAngle(VRTRIX::Thumb_Intermediate, eIMUError);
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

		}

		//If j is pressed, print out left hand index finger & middle finger yaw angle
		if (key == 'j') {
			if (pLeftHandDataGloves.size() >= 1) {
				double index_middle = pLeftHandDataGloves[0]->GetFingerYawAngle(VRTRIX::Middle_Intermediate, VRTRIX::Index_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}

				double middle_ring = pLeftHandDataGloves[0]->GetFingerYawAngle(VRTRIX::Ring_Intermediate, VRTRIX::Middle_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}

				double ring_pinky = pLeftHandDataGloves[0]->GetFingerYawAngle(VRTRIX::Pinky_Intermediate, VRTRIX::Ring_Intermediate, eIMUError);
				if (eIMUError != VRTRIX::IMUError_None) {
					std::cout << "IMU error: " << eIMUError << std::endl;
					continue;
				}

				std::cout << "Index_Middle Spacing: " << index_middle << " Middle_Ring Spacing: " << middle_ring << " Ring_Pinky Spacing: " << ring_pinky << std::endl;
			}
		}

		//If k is pressed, print out current wrist pitch angle.
		if (key == 'k') {
			Eigen::Quaterniond wristQuat = VRTRIXQuat2EigenQuat(lhData[VRTRIX::Wrist_Joint]);
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
			std::cout << "Left hand wrist pitch angle: " << pitch << std::endl;
		}

		//If l is pressed, prepre firmware update for left hand dongle
		if (key == 'l') {
			std::cout << "Update Left Hand Dongle Firmware Prepare" << std::endl;
			if (pLeftHandDataGloves.size() >= 1) {
				pLeftHandDataGloves[0]->PrepareFirmwareUpdate(eIMUError, VRTRIX::Device_USBDongle);
			}
		}

		//If m is pressed, start firmware update for left hand dongle
		if (key == 'm') {
			std::cout << "Update Left Hand Dongle Firmware Start" << std::endl;
			VRTRIX::EIMUError eIMUError;
			if (pLeftHandDataGloves.size() >= 1) {
				pLeftHandDataGloves[0]->StartUpdateFirmware(eIMUError, "glove_rx_pro6.zip", VRTRIX::Device_USBDongle);
				Sleep(100);
				for (int i = 0; i < gloveNum; ++i) {
					VRTRIX::IVRTRIXIMU* pDataGlove;
					if (glovePorts[i].type == VRTRIX::Hand_Left) {
						InitDataGloveStreaming(glovePorts[i], pLeftHandDataGloves[0]);
						break;
					}
				}
			}
		}

		//If n is pressed, prepre firmware update for left hand glove
		if (key == 'n') {
			std::cout << "Update Left Hand Glove Firmware Prepare" << std::endl;
			if (pLeftHandDataGloves.size() >= 1) {
				pLeftHandDataGloves[0]->PrepareFirmwareUpdate(eIMUError, VRTRIX::Device_Glove);
				std::cout << "Prepare error: " << eIMUError << std::endl;
			}
		}

		//If o is pressed, start firmware update for left hand glove
		if (key == 'o') {
			std::cout << "Update Left Hand Glove Firmware Start" << std::endl;
			VRTRIX::EIMUError eIMUError;
			if (pLeftHandDataGloves.size() >= 1) {
				pLeftHandDataGloves[0]->StartUpdateFirmware(eIMUError, "glove_tx_pro6.zip", VRTRIX::Device_Glove);
			}
		}
	}

	return 0;
}

