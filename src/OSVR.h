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

private:
	OpenSourceVirtualReality(std::string applicationIdentifier, bool serverAutoStart);

	void threadFunction(bool serverAutoStart);

	std::thread thd;
	std::mutex mtx;
	std::string app_identifier = "";
	bool is_thread_running = true;
	int fps = 60;

	std::deque<std::string> interface_paths;
	std::map<std::string, osvr::clientkit::Interface> interfaces;

};
