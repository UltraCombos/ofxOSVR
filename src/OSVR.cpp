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
	printf("osvr clear\n");
}

void OpenSourceVirtualReality::threadFunction(bool serverAutoStart)
{
	if (serverAutoStart)
	{
		printf("osvrClientAttemptServerAutoStart\n");
		osvrClientAttemptServerAutoStart();
	}

	osvr::clientkit::ClientContext ctx(app_identifier.c_str(), 0);
	osvr::clientkit::DisplayConfig display;

	//osvrRegisterPoseCallback(head.get(), directionCallback, NULL);
	//head.registerCallback(&OpenSourceVirtualReality::poseCallback, NULL);
	//head.registerCallback(&orientationCallback, NULL);
	//head.registerCallback(&positionCallback, NULL);

	//printf("osvr while loop\n");
	while (is_thread_running)
	{
		{
			std::lock_guard<std::mutex> guard(mtx);
			if (interface_paths.size() > 0)
			{
				for (auto& path : interface_paths)
				{
					printf("add interface: %s\n", path.c_str());
					interfaces[path] = ctx.getInterface(path);
				}
				interface_paths.clear();
			}
		}

		ctx.update();

		// display
		display = osvr::clientkit::DisplayConfig(ctx);

		if (display.valid())
		{
			display.forEachEye([](osvr::clientkit::Eye eye)
			{
				double view_mat[OSVR_MATRIX_SIZE];
				eye.getViewMatrix(OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS, view_mat);
				// apply model view matrix


				eye.forEachSurface([](osvr::clientkit::Surface surface)
				{
					auto viewport = surface.getRelativeViewport();
					// apply viewport

					double z_near = 0.1;
					double z_far = 100;
					double proj_mat[OSVR_MATRIX_SIZE];
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
		}


		// interface state
		for (auto& f : interfaces)
		{
			osvr::clientkit::Interface curr_interface = f.second;
			printf("check for interface: %s\n", f.first.c_str());
			OSVR_PoseState state;
			OSVR_TimeValue timestamp;
			OSVR_ReturnCode ret = osvrGetPoseState(curr_interface.get(), &timestamp, &state);
			if (ret != OSVR_RETURN_SUCCESS) {
				printf("No pose state!\n");
			}
			else {
				printf("Got POSE state: Position = (%f, %f, %f), orientation = (%f, %f, %f, %f)\n",
					state.translation.data[0], 
					state.translation.data[1],
					state.translation.data[2],
					osvrQuatGetW(&(state.rotation)),
					osvrQuatGetX(&(state.rotation)),
					osvrQuatGetY(&(state.rotation)),
					osvrQuatGetZ(&(state.rotation)));
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
	}
}

// only for callback function template
namespace
{
	void poseCallback(void *userdata, const OSVR_TimeValue *timestamp,
		const OSVR_PoseReport *report) {
		printf("Got POSE report: Position = (%f, %f, %f), orientation = (%f, %f, %f, %f)\n",
			report->pose.translation.data[0], report->pose.translation.data[1],
			report->pose.translation.data[2],
			osvrQuatGetW(&(report->pose.rotation)),
			osvrQuatGetX(&(report->pose.rotation)),
			osvrQuatGetY(&(report->pose.rotation)),
			osvrQuatGetZ(&(report->pose.rotation)));
	}

	void orientationCallback(void *userdata, const OSVR_TimeValue *timestamp,
		const OSVR_OrientationReport *report) {
		printf("Got ORIENTATION report: Orientation = (%f, %f, %f, %f)\n",
			osvrQuatGetW(&(report->rotation)), osvrQuatGetX(&(report->rotation)),
			osvrQuatGetY(&(report->rotation)),
			osvrQuatGetZ(&(report->rotation)));
	}

	void positionCallback(void *userdata, const OSVR_TimeValue *timestamp,
		const OSVR_PositionReport *report) {
		printf("Got POSITION report: Position = (%f, %f, %f)\n",
			report->xyz.data[0], report->xyz.data[1], report->xyz.data[2]);
	}

	void printDirectionReport(const OSVR_DirectionReport *report) {

		std::cout << report->direction.data[0] << "; " << report->direction.data[1]
			<< "; " << report->direction.data[2] << "\t" << std::endl;
	}

	void directionCallback(void * /*userdata*/,
		const OSVR_TimeValue * /*timestamp*/,
		const OSVR_DirectionReport *report) {
		std::cout << "Got 3D direction Report, for sensor #" << report->sensor
			<< std::endl;
		printDirectionReport(report);
	}
}