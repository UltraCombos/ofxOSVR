#include "OSVR.h"

// ignore conflict define
#pragma push_macro("ignore")
#undef near
#undef far
#include "osvr/ClientKit/Context.h"
#include "osvr/ClientKit/Display.h"
#include "osvr/ClientKit/DisplayC.h"
#include "osvr/ClientKit/ServerAutoStartC.h"
#include "osvr/ClientKit/InterfaceStateC.h"
#pragma pop_macro("ignore")

using namespace std;

OpenSourceVirtualReality::OpenSourceVirtualReality(string applicationIdentifier, bool serverAutoStart)
	:app_identifier(applicationIdentifier)
{
	thd = std::thread(&OpenSourceVirtualReality::threadFunction, this, serverAutoStart);
}

OpenSourceVirtualReality::~OpenSourceVirtualReality()
{
	is_thread_running = false;
	thd.join();
	ofLogNotice(module, "clear");
}

void OpenSourceVirtualReality::threadFunction(bool serverAutoStart)
{
	if (serverAutoStart)
	{
		ofLogNotice(module, "client attempt server auto start");
		osvrClientAttemptServerAutoStart();
	}

	osvr::clientkit::ClientContext ctx(app_identifier.c_str(), 0);
	osvr::clientkit::DisplayConfig display(ctx);

	// check display valid
	if (display.valid() == false) {
		ofLogError(module, "Could not get display config (server probably not running or not behaving), exiting.");
		is_thread_running = false;
	}
	else
	{
		// check display startup
		ofLogNotice(module, "display check startup");
		float check_time_out = 5.0f;
		float check_timestamp = ofGetElapsedTimef();
		while (display.checkStartup() == false) {
			ctx.update();
			float dt = ofGetElapsedTimef() - check_timestamp;
			if (dt > check_time_out)
			{
				is_thread_running = false;
				ofLogWarning(module, "display check time out");
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (is_thread_running)
			ofLogNotice(module, "display startup status is good");
	}


	while (is_thread_running)
	{
		{
			std::lock_guard<std::mutex> guard(mtx);
			if (interface_paths.size() > 0)
			{
				for (auto& path : interface_paths)
				{
					ofLogNotice(module, "interface add: %s\n", path.c_str());

					interfaces[path] = InterfaceInfo();
					InterfaceInfo& info = interfaces[path];
					info.interface = ctx.getInterface(path);
					//info.interface.registerCallback((OSVR_PoseCallback)OpenSourceVirtualReality::poseCallback, this);
				}
				interface_paths.clear();
			}
		}

		ctx.update();

		printf("display valid\n");
		printf("%u viewers\n", display.getNumViewers());
		int num_eye = 0;
		display.forEachEye([&](osvr::clientkit::Eye eye)
		{
			num_eye++;
			int num_suf = 0;
			eye.forEachSurface([&](osvr::clientkit::Surface surface)
			{
				num_suf++;
			});
			printf("%u surfaces\n", num_suf);
		});
		printf("%u eye\n", num_eye);
#if 0
		display.forEachEye([&](osvr::clientkit::Eye eye)
		{
			float view_mat[OSVR_MATRIX_SIZE];
			if (eye.getViewMatrix(OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS, view_mat))
			{
				// apply model view matrix
				//model_view_matrix;

				memcpy(model_view_matrix.getPtr(), view_mat, sizeof(float) * OSVR_MATRIX_SIZE);
			}

			eye.forEachSurface([](osvr::clientkit::Surface surface)
			{
				auto viewport = surface.getRelativeViewport();
				// apply viewport

				float z_near = 0.1;
				float z_far = 100;
				float proj_mat[OSVR_MATRIX_SIZE];
				surface.getProjectionMatrix(z_near, z_far, OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS | OSVR_MATRIX_SIGNEDZ | OSVR_MATRIX_RHINPUT, proj_mat);
				// apply projection matrix

			});
		});

		for (uint32_t i = 0; i < display.getNumViewers(); i++)
		{
			osvr::clientkit::Viewer viewer = display.getViewer(i);
			std::cout << "Viewer " << viewer.getViewerID() << "\n";

			viewer.forEachEye([](osvr::clientkit::Eye eye) {
				std::cout << "\tEye " << int(eye.getEyeID()) << "\n";
				eye.forEachSurface([](osvr::clientkit::Surface surface) {
					std::cout << "\t\tSurface " << surface.getSurfaceID() << "\n";
				});
			});
		}
#endif

		// interface state
		for (auto& f : interfaces)
		{
			printf("check for interface: %s\n", f.first.c_str());
			InterfaceInfo& info = f.second;
			OSVR_ReturnCode ret = osvrGetPoseState(info.interface.get(), &info.timestamp, &info.state);
			if (ret != OSVR_RETURN_SUCCESS) {
				printf("No pose state!\n");
			}
			else {
				std::lock_guard<std::mutex> guard(mtx);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
	}

	ofLogNotice(module, "thread exit");
}

void OpenSourceVirtualReality::poseCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_PoseReport *report)
{
	printf("Got POSE report: Position = (%f, %f, %f), orientation = (%f, %f, %f, %f)\n",
		report->pose.translation.data[0], 
		report->pose.translation.data[1],
		report->pose.translation.data[2],
		osvrQuatGetW(&(report->pose.rotation)),
		osvrQuatGetX(&(report->pose.rotation)),
		osvrQuatGetY(&(report->pose.rotation)),
		osvrQuatGetZ(&(report->pose.rotation)));

	OpenSourceVirtualReality* ptr = (OpenSourceVirtualReality*)userdata;

}

void OpenSourceVirtualReality::orientationCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_OrientationReport *report)
{
	printf("Got ORIENTATION report: Orientation = (%f, %f, %f, %f)\n",
		osvrQuatGetW(&(report->rotation)), 
		osvrQuatGetX(&(report->rotation)),
		osvrQuatGetY(&(report->rotation)),
		osvrQuatGetZ(&(report->rotation)));

	OpenSourceVirtualReality* ptr = (OpenSourceVirtualReality*)userdata;
}

void OpenSourceVirtualReality::positionCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_PositionReport *report)
{
	printf("Got POSITION report: Position = (%f, %f, %f)\n",
		report->xyz.data[0], report->xyz.data[1], report->xyz.data[2]);

	OpenSourceVirtualReality* ptr = (OpenSourceVirtualReality*)userdata;
}



// only for callback function template
namespace
{
	void printDirectionReport(const OSVR_DirectionReport *report) {

		std::cout << report->direction.data[0] << "; " << report->direction.data[1]
			<< "; " << report->direction.data[2] << "\t" << std::endl;
	}

	void directionCallback(void * /*userdata*/, const OSVR_TimeValue * /*timestamp*/, const OSVR_DirectionReport *report) {
		std::cout << "Got 3D direction Report, for sensor #" << report->sensor
			<< std::endl;
		printDirectionReport(report);
	}
}