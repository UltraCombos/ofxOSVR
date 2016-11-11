#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <deque>
#include <map>

#include "ofMain.h"
#include "osvr/ClientKit/Interface.h"

using OpenSourceVirtualRealityRef = std::shared_ptr<class OpenSourceVirtualReality>;

class OpenSourceVirtualReality
{
public:
	static OpenSourceVirtualRealityRef create(std::string applicationIdentifier, bool serverAutoStart = true)
	{
		return OpenSourceVirtualRealityRef(new OpenSourceVirtualReality(applicationIdentifier, serverAutoStart));
	}

	~OpenSourceVirtualReality();

	void addInterface(std::string path)
	{
		std::lock_guard<std::mutex> guard(mtx);
		interface_paths.push_back(path);
	}

	bool getInterfacePose(const std::string& path, ofVec3f& translation, ofQuaternion& rotation)
	{
		std::lock_guard<std::mutex> guard(mtx);
		if (interface_infos.find(path) == interface_infos.end())
		{
			ofLogWarning(module, "interface is not found with path: %s", path.c_str());
			return false;
		}
			

		auto& state = interface_infos[path].state;
		double x, y, z, w;

		x = state.translation.data[0];
		y = state.translation.data[1];
		z = state.translation.data[2];
		translation.set(x, y, z);

		x = osvrQuatGetX(&(state.rotation));
		y = osvrQuatGetY(&(state.rotation));
		z = osvrQuatGetZ(&(state.rotation));
		w = osvrQuatGetW(&(state.rotation));
		rotation.set(x, y, z, w);

		return true;
	}

	struct Surface
	{
		ofMatrix4x4 projection_matrix;
		ofRectangle viewport;
	};

	struct Eye
	{
		ofMatrix4x4 modelview_matrix;
		std::map<uint32_t, Surface> surfaces;
	};

	struct Viewer
	{
		std::map<uint32_t, Eye> eyes;
	};

protected:
	struct InterfaceInfo
	{	
		InterfaceInfo() {}
		InterfaceInfo(osvr::clientkit::Interface face)
			:interface(face)
		{

		}
		osvr::clientkit::Interface interface;		
		OSVR_PoseState state;
		OSVR_TimeValue timestamp;
	};

private:
	OpenSourceVirtualReality(std::string applicationIdentifier, bool serverAutoStart);
	
	void threadFunction(bool serverAutoStart);

	static void poseCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_PoseReport *report);
	static void orientationCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_OrientationReport *report);
	static void positionCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_PositionReport *report);

	std::thread thd;
	std::mutex mtx;
	std::string app_identifier = "";
	bool is_thread_running = true;
	int fps = 60;
	const string module = "OSVR";

	ofMatrix4x4 modelview_matrix;
	ofRectangle viewport;
	ofMatrix4x4 projection_matrix;

	std::deque<std::string> interface_paths;
	//std::map<std::string, osvr::clientkit::Interface> interfaces;
	std::map<std::string, InterfaceInfo> interface_infos;
	std::map<uint32_t, Viewer> viewers;
};
