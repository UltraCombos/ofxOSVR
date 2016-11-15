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
	osvrClientReleaseAutoStartedServer();
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
					interface_infos[path] = InterfaceInfo(ctx.getInterface(path));
				}
				interface_paths.clear();
			}
		}

		ctx.update();

		{
			std::lock_guard<std::mutex> guard(mtx);
			for (uint32_t i = 0; i < display.getNumViewers(); i++)
			{
				auto& viewer = display.getViewer(i);
				auto viewer_id = viewer.getViewerID();
				if (viewers.find(viewer_id) == viewers.end())
					viewers[viewer_id] = Viewer();
				auto& curr_viewer = viewers[viewer_id];
				for (uint32_t j = 0; j < viewer.getNumEyes(); j++)
				{
					auto& eye = viewer.getEye(j);
					auto eye_id = eye.getEyeID();
					if (curr_viewer.eyes.find(eye_id) == curr_viewer.eyes.end())
						curr_viewer.eyes[eye_id] = Eye();
					auto& curr_eye = curr_viewer.eyes[eye_id];

					float view_mat[OSVR_MATRIX_SIZE];
					if (eye.getViewMatrix(OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS, view_mat))
					{
						memcpy(curr_eye.modelview_matrix.getPtr(), view_mat, sizeof(float) * OSVR_MATRIX_SIZE);
					}

					for (uint32_t k = 0; k < eye.getNumSurfaces(); k++)
					{
						auto& surface = eye.getSurface(k);
						auto surface_id = surface.getSurfaceID();
						if (curr_eye.surfaces.find(surface_id) == curr_eye.surfaces.end())
							curr_eye.surfaces[surface_id] = Surface();
						auto& curr_surface = curr_eye.surfaces[surface_id];

						auto viewport = surface.getRelativeViewport();
						curr_surface.viewport.set(viewport.left, viewport.bottom, viewport.width, viewport.height);

						float z_near = 0.1;
						float z_far = 100;
						float proj_mat[OSVR_MATRIX_SIZE];
						surface.getProjectionMatrix(z_near, z_far, OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS | OSVR_MATRIX_SIGNEDZ | OSVR_MATRIX_RHINPUT, proj_mat);
						memcpy(curr_surface.projection_matrix.getPtr(), proj_mat, sizeof(float) * OSVR_MATRIX_SIZE);
					}
				}
			}
		}
		
#if 0
		int num_eye = 0;
		display.getViewer(0).getEye(0).getSurface(0);
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
		{
			std::lock_guard<std::mutex> guard(mtx);
			for (auto& f : interface_infos)
			{
				std::printf("check for interface: %s\n", f.first.c_str());
				InterfaceInfo& info = f.second;
				OSVR_ReturnCode ret = osvrGetPoseState(info.interface.get(), &info.timestamp, &info.state);
				if (ret != OSVR_RETURN_SUCCESS) {
					std::printf("No pose state!\n");
				}
				else {

				}
			}
		}
		

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
	}

	ofLogNotice(module, "thread exit");
}

bool OpenSourceVirtualReality::getInterfacePose(const string& path, ofVec3f& translation, ofQuaternion& rotation)
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

void OpenSourceVirtualReality::poseCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_PoseReport *report)
{
	std::printf("Got POSE report: Position = (%f, %f, %f), orientation = (%f, %f, %f, %f)\n",
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
	std::printf("Got ORIENTATION report: Orientation = (%f, %f, %f, %f)\n",
		osvrQuatGetW(&(report->rotation)), 
		osvrQuatGetX(&(report->rotation)),
		osvrQuatGetY(&(report->rotation)),
		osvrQuatGetZ(&(report->rotation)));

	OpenSourceVirtualReality* ptr = (OpenSourceVirtualReality*)userdata;
}

void OpenSourceVirtualReality::positionCallback(void *userdata, const OSVR_TimeValue *timestamp, const OSVR_PositionReport *report)
{
	std::printf("Got POSITION report: Position = (%f, %f, %f)\n",
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