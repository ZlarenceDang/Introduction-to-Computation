/*
* Copyright (c) 2006-2016 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

// This is altered from the original BOX2D testbed to simulate the evolution of a car.

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <glew/glew.h>
#endif

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include "DebugDraw.h"
#include "Test.h"

#include "glfw/glfw3.h"
#include <stdio.h>
#include <vector>
#include "main\Evolution\showcar.h"
#include<sstream>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

// This include was added to support MinGW
#ifdef _WIN32
#include <crtdbg.h>
#endif

//total gene number
constexpr int gene_count{ 50 };

const char importDir[] = "../../import_data/";
const char outputDir[] = "../../output_data/";
const char preciousDir[] = "../../precious/";

struct
{
	//shall we begin
	bool SelectMode = true;
	bool needToLoadMainArena = false;
	//whether to draw everything
	bool show_dynamic = true;

	//gravity
	float gravity = 9.8f;
	bool gravity_storm_on = false;

	//terrain change
	bool switch_terrain = false;
	bool always_switch_terrain = false;

	//choose how to evaluate fitness
	//DISTANCE = 1, SPEED, FANCY, JUMP, CLIMB
	static constexpr fitness_judge defaultJudge = WELCOME;
	fitness_judge how_to_judge = defaultJudge;
	static constexpr int modeCount = 6;

	//show flags
	bool  show_flags = true;

	//open logs
	bool create_log = true;
	bool checkOverWrite = false;

	//import
	bool foundImport = false;
	const char * importFileName;

}global_settings;

struct UIState
{
	bool showMenu;
	bool showSelect;
};

//
namespace
{
	GLFWwindow* mainWindow = NULL;
	UIState ui;

	int32 testIndex = 0;
	int32 testSelection = 0;
	int32 testCount = 0;
	TestEntry* entry;
	Test* test;
	Settings settings;
	bool rightMouseDown;
	b2Vec2 lastp;

	//gene input_gene = rand_gene();
}

//
static void sCreateUI(GLFWwindow* window)
{
	//ui.showMenu = true;
	ui.showMenu = false;
	ui.showSelect = true;

	// Init UI
	char tmpName[50];
	strcpy(tmpName, preciousDir);
	strcat(tmpName, "DroidSans.ttf");
	const char* fontPath = tmpName;
	ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath, 15.f);

	if (ImGui_ImplGlfwGL3_Init(window, false) == false)
	{
		fprintf(stderr, "Could not init GUI renderer.\n");
		assert(false);
		return;
	}

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = style.GrabRounding = style.ScrollbarRounding = 2.0f;
	style.FramePadding = ImVec2(4, 2);
	style.DisplayWindowPadding = ImVec2(0, 0);
	style.DisplaySafeAreaPadding = ImVec2(0, 0);
}

//
static void sResizeWindow(GLFWwindow*, int width, int height)
{
	g_camera.m_width = width;
	g_camera.m_height = height;
}

//
static void sKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);

	bool keys_for_ui = ImGui::GetIO().WantCaptureKeyboard;
	if (keys_for_ui) return;
	if (global_settings.SelectMode) return; 

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			// Quit
			glfwSetWindowShouldClose(mainWindow, GL_TRUE);
			break;

		case GLFW_KEY_LEFT:
			// Pan left
			if (mods == GLFW_MOD_CONTROL)
			{
				//b2Vec2 newOrigin(2.0f, 0.0f);
				//test->ShiftOrigin(newOrigin);
				g_camera.m_center.x -= 10.0f * g_camera.m_zoom;
			}
			else if (mods == GLFW_MOD_SHIFT)
			{
				g_camera.m_center.x -= 100.0f * g_camera.m_zoom;
			}
			else
			{
				//g_camera.m_center.x -= 0.5f;
				g_camera.m_center.x -= 40.0f * g_camera.m_zoom;
			}
			break;

		case GLFW_KEY_RIGHT:
			// Pan right
			if (mods == GLFW_MOD_CONTROL)
			{
				//b2Vec2 newOrigin(-2.0f, 0.0f);
				//test->ShiftOrigin(newOrigin);
				g_camera.m_center.x += 10.0f * g_camera.m_zoom;
			}
			else if (mods == GLFW_MOD_SHIFT)
			{
				g_camera.m_center.x += 100.0f * g_camera.m_zoom;
			}
			else
			{
				//g_camera.m_center.x += 0.5f;
				g_camera.m_center.x += 40.0f * g_camera.m_zoom;
			}

			break;

		case GLFW_KEY_DOWN:
			// Pan down
			if (mods == GLFW_MOD_CONTROL)
			{
				//b2Vec2 newOrigin(0.0f, 2.0f);
				//test->ShiftOrigin(newOrigin);
				g_camera.m_center.y -= 2.5f * g_camera.m_zoom;
			}
			else if (mods == GLFW_MOD_SHIFT)
			{
				g_camera.m_center.y -= 25.0f * g_camera.m_zoom;
			}
			else
			{
				//g_camera.m_center.y -= 0.5f;
				g_camera.m_center.y -= 10.0f * g_camera.m_zoom;
			}
			break;

		case GLFW_KEY_UP:
			// Pan up
			if (mods == GLFW_MOD_CONTROL)
			{
				//b2Vec2 newOrigin(0.0f, -2.0f);
				//test->ShiftOrigin(newOrigin);
				g_camera.m_center.y += 2.5f * g_camera.m_zoom;
			}
			else if (mods == GLFW_MOD_SHIFT)
			{
				g_camera.m_center.y += 25.0f * g_camera.m_zoom;
			}
			else
			{
				//g_camera.m_center.y += 0.5f;
				g_camera.m_center.y += 10.0f * g_camera.m_zoom;
			}
			break;

		case GLFW_KEY_Z:
			// Zoom out
			g_camera.m_zoom = b2Min(1.1f * g_camera.m_zoom, 20.0f);
			break;

		case GLFW_KEY_X:
			// Zoom in
			g_camera.m_zoom = b2Max(0.9f * g_camera.m_zoom, 0.02f);
			break;

		case GLFW_KEY_R:
			// Reset test
			delete test;
			//test = entry->createFcn();
			test = new evo_world{ rand_gene(gene_count) ,global_settings.how_to_judge};
			break;

		case GLFW_KEY_SPACE:
			// Launch a bomb.
			if (test)
			{
				test->LaunchBomb();
			}
			break;

		case GLFW_KEY_CAPS_LOCK:
			global_settings.show_dynamic = !global_settings.show_dynamic;
			break;

		//case GLFW_KEY_O:
		//	settings.singleStep = true;
		//	break;

		case GLFW_KEY_P:
			settings.pause = !settings.pause;
			break;

		//case GLFW_KEY_LEFT_BRACKET:
		//	// Switch to previous test
		//	--testSelection;
		//	if (testSelection < 0)
		//	{
		//		testSelection = testCount - 1;
		//	}
		//	break;
		//
		//case GLFW_KEY_RIGHT_BRACKET:
		//	// Switch to next test
		//	++testSelection;
		//	if (testSelection == testCount)
		//	{
		//		testSelection = 0;
		//	}
		//	break;

		case GLFW_KEY_TAB:
			ui.showMenu = !ui.showMenu;
			break;

		case GLFW_KEY_0:
			settings.detailinfo = !settings.detailinfo;
			break;

		case GLFW_KEY_1:
			settings.fix_camera = !settings.fix_camera;
			break;

		default:
			if (test)
			{
				test->Keyboard(key);
			}
		}
	}
	else if (action == GLFW_RELEASE)
	{
		test->KeyboardUp(key);
	}
	// else GLFW_REPEAT
}

//
static void sCharCallback(GLFWwindow* window, unsigned int c)
{
	ImGui_ImplGlfwGL3_CharCallback(window, c);
}

//
static void sMouseButton(GLFWwindow* window, int32 button, int32 action, int32 mods)
{
	ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);

	double xd, yd;
	glfwGetCursorPos(mainWindow, &xd, &yd);
	b2Vec2 ps((float32)xd, (float32)yd);

	// Use the mouse to move things around.
	if (button == GLFW_MOUSE_BUTTON_1)
	{
        //<##>
        //ps.Set(0, 0);
		b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
		if (action == GLFW_PRESS)
		{
			if (mods == GLFW_MOD_SHIFT)
			{
				test->ShiftMouseDown(pw);
			}
			else
			{
				test->MouseDown(pw);
			}
		}
		
		if (action == GLFW_RELEASE)
		{
			test->MouseUp(pw);
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_2)
	{
		if (action == GLFW_PRESS)
		{	
			lastp = g_camera.ConvertScreenToWorld(ps);
			rightMouseDown = true;
		}

		if (action == GLFW_RELEASE)
		{
			rightMouseDown = false;
		}
	}
}

//
static void sMouseMotion(GLFWwindow*, double xd, double yd)
{
	b2Vec2 ps((float)xd, (float)yd);

	b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
	test->MouseMove(pw);
	
	if (rightMouseDown)
	{
		b2Vec2 diff = pw - lastp;
		g_camera.m_center.x -= diff.x;
		g_camera.m_center.y -= diff.y;
		lastp = g_camera.ConvertScreenToWorld(ps);
	}
}

//
static void sScrollCallback(GLFWwindow* window, double dx, double dy)
{
	ImGui_ImplGlfwGL3_ScrollCallback(window, dx, dy);
	bool mouse_for_ui = ImGui::GetIO().WantCaptureMouse;
	if (global_settings.SelectMode) return;
	if (!mouse_for_ui)
	{
		if (dy > 0)
		{
			g_camera.m_zoom /= 1.1f;
		}
		else
		{
			g_camera.m_zoom *= 1.1f;
		}
	}
}

//
static void sRestart(vector<gene> genel)
{
	delete test;
	entry = g_testEntries + testIndex;
	//test = entry->createFcn();
	test = new evo_world{ genel ,global_settings.how_to_judge};
}

//
static void sSimulate()
{
	glEnable(GL_DEPTH_TEST);
	test->Step(&settings);

	test->DrawTitle(entry->name);
	glDisable(GL_DEPTH_TEST);

	if (testSelection != testIndex)
	{
		testIndex = testSelection;
		delete test;
		entry = g_testEntries + testIndex;
		//test = entry->createFcn();
		test = new evo_world{ rand_gene(gene_count) ,global_settings.how_to_judge};
		g_camera.m_zoom = 1.0f;
		g_camera.m_center.Set(0.0f, 20.0f);
	}
}

//
static bool sTestEntriesGetName(void*, int idx, const char** out_name)
{
	*out_name = g_testEntries[idx].name;
	return true;
}

//
static void sInterface()
{
	int menuWidth = 250;
	if (ui.showMenu)
	{
		ImGui::SetNextWindowPos(ImVec2((float)g_camera.m_width - menuWidth - 10, 10));
		ImGui::SetNextWindowSize(ImVec2((float)menuWidth, (float)g_camera.m_height - 20));
		ImGui::Begin("Testbed Controls", &ui.showMenu, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
		ImGui::PushAllowKeyboardFocus(false); // Disable TAB

		ImGui::PushItemWidth(-1.0f);

		//ImGui::Text("Test");
		//if (ImGui::Combo("##Test", &testIndex, sTestEntriesGetName, NULL, testCount, testCount))
		//{
		//	delete test;
		//	entry = g_testEntries + testIndex;
		//	//test = entry->createFcn();
		//	test = new evo_world{ rand_gene(gene_count) ,global_settings.how_to_judge};
		//	testSelection = testIndex;
		//}
		//ImGui::Separator();

		ImGui::Text("Set gravity!");
		ImGui::SliderFloat("##Gravity", &global_settings.gravity, -5.0f, 20.0f, "%.2f m/s^2");
		ImGui::Checkbox("!!!GRAVITY STORM!!!", &global_settings.gravity_storm_on);
		ImGui::Separator();
		ImGui::Checkbox("Change Terrain!", &global_settings.switch_terrain);
		ImGui::Checkbox("CONSTANTLY!!!", &global_settings.always_switch_terrain);
		ImGui::Separator();
		ImGui::Checkbox("Show flags", &global_settings.show_flags);
		ImGui::Checkbox("Show detailed info.", &settings.detailinfo);
		ImGui::Checkbox("Fix camera on the 1st car", &settings.fix_camera);
		ImGui::Separator();


		//ImGui::Text("Vel Iters");
		//ImGui::SliderInt("##Vel Iters", &settings.velocityIterations, 0, 50);
		//ImGui::Text("Pos Iters");
		//ImGui::SliderInt("##Pos Iters", &settings.positionIterations, 0, 50);
		ImGui::Text("Hertz");
		ImGui::SliderFloat("##Hertz", &settings.hz, 5.0f, 120.0f, "%.0f hz");
		ImGui::PopItemWidth();

		//ImGui::Checkbox("Sleep", &settings.enableSleep);
		//ImGui::Checkbox("Warm Starting", &settings.enableWarmStarting);
		//ImGui::Checkbox("Time of Impact", &settings.enableContinuous);
		//ImGui::Checkbox("Sub-Stepping", &settings.enableSubStepping);
		//
		//ImGui::Separator();
		//
		//ImGui::Checkbox("Shapes", &settings.drawShapes);
		//ImGui::Checkbox("Joints", &settings.drawJoints);
		//ImGui::Checkbox("AABBs", &settings.drawAABBs);
		//ImGui::Checkbox("Contact Points", &settings.drawContactPoints);
		//ImGui::Checkbox("Contact Normals", &settings.drawContactNormals);
		//ImGui::Checkbox("Contact Impulses", &settings.drawContactImpulse);
		//ImGui::Checkbox("Friction Impulses", &settings.drawFrictionImpulse);
		//ImGui::Checkbox("Center of Masses", &settings.drawCOMs);
		//ImGui::Checkbox("Statistics", &settings.drawStats);
		//ImGui::Checkbox("Profile", &settings.drawProfile);

		ImVec2 button_sz = ImVec2(-1, 0);
		if (ImGui::Button("Pause (P)", button_sz))
			settings.pause = !settings.pause;

		if (ImGui::Button("Single Step (O)", button_sz))
			settings.singleStep = !settings.singleStep;

		if (ImGui::Button("Restart (R)", button_sz))
			sRestart(rand_gene(gene_count));

		if (ImGui::Button("Quit", button_sz))
			glfwSetWindowShouldClose(mainWindow, GL_TRUE);

		ImGui::PopAllowKeyboardFocus();
		ImGui::End();
	}
	//ImGui::ShowTestWindow(NULL);
}

static void sSelection()
{
	float menuWidth = g_camera.m_width*0.35f, menuHeight = g_camera.m_height * 0.25f;
	if (ui.showSelect = global_settings.SelectMode)
	{
		ImGui::SetNextWindowPos(ImVec2((float)g_camera.m_width/2 - menuWidth/2,g_camera.m_height*0.62));
		ImGui::SetNextWindowSize(ImVec2((float)menuWidth, (float)menuHeight));
		ImGui::Begin("Car evolution", &ui.showSelect, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::PushAllowKeyboardFocus(false); // Disable TAB

		ImGui::Text("Welcome to Car Evolution Simulation\n");
		ImGui::Text("We should add more crap here as an introduction\n");
		
		static bool importData = false;
		ImGui::Checkbox("Import external data file as initial settings", &importData);
		
		if (importData)
		{
			static bool enterPressed = false;
			static char inFileName[200] = "";
			static char boxText[128] = "";
			if (ImGui::InputText("press enter to confirm", boxText, 128, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				strcpy(inFileName, importDir);
				strcat(inFileName, boxText);
				ifstream ImportData{ inFileName };
				if (global_settings.foundImport = (bool)ImportData)
				{
					global_settings.importFileName = inFileName;
				}
				enterPressed = true;
			}
			if (enterPressed)
			{
				if (global_settings.foundImport) ImGui::Text("Import data file has been successfully found:P");
				else ImGui::Text("Fail to locate the specified file.");
			}
		}
		else global_settings.foundImport = false;

		ImGui::Checkbox("Create Logs", &global_settings.create_log);
		ImGui::Checkbox("Prevent overwriting existing data files", &global_settings.checkOverWrite);

		static int currentMode = 0;
		if (ImGui::Combo("is the Current Mode", &currentMode, getModeName, NULL, global_settings.modeCount, global_settings.modeCount))
		{
			//do something if user has chosen currentMode 
		}

		ImVec2 ButSize(-1.0f, 0.0f);

		ImGui::Separator();

		if (ImGui::Button("Start", ButSize))
		{
			global_settings.how_to_judge = (fitness_judge)currentMode;
			global_settings.SelectMode = false;
			global_settings.needToLoadMainArena = true;
			ui.showMenu = true;
		}
		
		if (ImGui::Button("Exit", ButSize))
		{
			glfwSetWindowShouldClose(mainWindow, GL_TRUE);
			return;
		}
		ImGui::End();
	}
}

void glfwErrorCallback(int error, const char *description)
{
	fprintf(stderr, "GLFW error occured. Code: %d. Description: %s\n", error, description);
}

bool TerrainLog(const vector<ppoint>& Terrain)
{
	fstream flog;
	char tmpName[50];
	strcpy(tmpName, outputDir);
	strcat(tmpName, "terrain.log");
	if (global_settings.checkOverWrite)
	{
		flog.open(tmpName, ios_base::in);
		if (flog)
		{
			cout << "existing terrain log file\n";
			getchar();
			return 0;
		}
		flog.clear();
		flog.close();
	}
	flog.open(tmpName, ios_base::out);
	flog << Terrain;
	flog.close();
	return 1;
}

bool CreateLog(const evolution & evo_main, const int & ind)
{
	fstream flog;
	char tmpName[50],id[10];
	strcpy(tmpName, outputDir);
	strcat(tmpName, "round");
	sprintf(id, "%d", ind);
	strcat(tmpName, id);
	strcat(tmpName, ".dat");
	if (global_settings.checkOverWrite)
	{
		flog.open(tmpName, ios_base::in);
		if (flog)
		{
			cout << "existing gene log file for round#" << evo_main.step << "\n";
			getchar();
			return 0;
		}
		flog.clear();
		flog.close();
	}
	flog.open(tmpName, ios_base::out);
	evo_main.echo(flog, ind);
	flog.close();
	return 1;
}

bool FitnessLog(const evolution &evo_main, const int &ind)
{
	char tmpName[50], id[10];
	strcpy(tmpName, outputDir);
	strcat(tmpName, "round");
	sprintf(id, "%d", ind);
	strcat(tmpName, id);
	strcat(tmpName, ".fitness");
	fstream flog;
	flog.open(tmpName, ios_base::out);
	flog << "overall data for round #" << ind << endl;
	flog << evo_main.fit_info[ind] << endl;
	return 1;
}

int main(int, char**)
{
	
#if defined(_WIN32)
	// Enable memory-leak reports
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

	glfwSetErrorCallback(glfwErrorCallback);

	g_camera.m_width = 1920;
	g_camera.m_height = 1080;
	g_camera.m_zoom /= 1.25;

	if (glfwInit() == 0)
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	char title[64];
	//sprintf(title, "Box2D Testbed Version %d.%d.%d", b2_version.major, b2_version.minor, b2_version.revision);
	sprintf(title, "Car Evolution based on Box2D Testbed Version %d.%d.%d", b2_version.major, b2_version.minor, b2_version.revision);

#if defined(__APPLE__)
	// Without these settings on macOS, OpenGL 2.1 will be used by default which will cause crashes at boot.
	// This code is a slightly modified version of the code found here: http://www.glfw.org/faq.html#how-do-i-create-an-opengl-30-context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	mainWindow = glfwCreateWindow(g_camera.m_width, g_camera.m_height, title, NULL, NULL);
	if (mainWindow == NULL)
	{
		fprintf(stderr, "Failed to open GLFW mainWindow.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(mainWindow);
	//printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	glfwSetScrollCallback(mainWindow, sScrollCallback);
	glfwSetWindowSizeCallback(mainWindow, sResizeWindow);
	glfwSetKeyCallback(mainWindow, sKeyCallback);
	glfwSetCharCallback(mainWindow, sCharCallback);
	glfwSetMouseButtonCallback(mainWindow, sMouseButton);
	glfwSetCursorPosCallback(mainWindow, sMouseMotion);
	glfwSetScrollCallback(mainWindow, sScrollCallback);

#if defined(__APPLE__) == FALSE
	//glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}
#endif

	g_debugDraw.Create();

	sCreateUI(mainWindow);

	testCount = 0;
	while (g_testEntries[testCount].createFcn != NULL)
	{
		++testCount;
	}

	testIndex = b2Clamp(testIndex, 0, testCount - 1);
	testSelection = testIndex;

	entry = g_testEntries + testIndex;
	
	char tmpName[50];
	strcpy(tmpName, preciousDir);
	strcat(tmpName, "BackGroundCar.inidat");
	ifstream initCar{ tmpName };
	if (!initCar)
	{
		cout << "fail to open background car file\n";
		assert(0);
	}
	vector<gene> initGene;
	initCar >> initGene;
	initCar.close();

    /*
	strcpy(tmpName, outputDir);
	strcat(tmpName, "initCar.out");
	ofstream checkFile{ tmpName };
	for (int i = 0; i < initGene.size(); i++)
	{
		checkFile << "gene #" << i << endl;
		checkFile << initGene[i];
	}
    */
	evolution evo_main(initGene);
//	evolution evo_main(rand_gene(gene_count));
	test = new evo_world{ evo_main.gene_pool ,global_settings.how_to_judge};

	//set terrain generation mode
	SetTerrainMode(global_settings.how_to_judge);
	((evo_world*)test)->create_terrain();

	//flags related
	vector<b2Vec2> flags = get_flag_position(FetchTerrain(), terrain_xmax,global_settings.how_to_judge);
	b2Color color_of_flags{ 0.0f,1.0f,0.0f,1.0f }; 
	//flag shape
	b2Vec2 flag_shape[5] = { { 0.0f, 0.0f },{ 0.05f, 0.0f },{ 0.035f, 0.015f },{ 0.05f, 0.03f },{ 0.0f, 0.03f } };
	b2Vec2 flag_shape_temp[5];


	if ((!global_settings.SelectMode) && (!global_settings.needToLoadMainArena) && global_settings.create_log)
	{
		if (!TerrainLog(FetchTerrain())) return 1;
	}

	// Control the frame rate. One draw per monitor refresh.
	glfwSwapInterval(1);

	double time1 = glfwGetTime();
	double frameTime = 0.0;
   
	glClearColor(0.3f, 0.3f, 0.3f, 1.f);

	//importing and initializing title image
	//drawing title!!!
	fstream title_file;
	strcpy(tmpName, preciousDir);
	strcat(tmpName, "title.inidat");
	title_file.open(tmpName, ios_base::in);
	//the filling part
	int title_character_count = -1;
	title_file >> title_character_count;

	int* character_vertex_count = (int*)calloc(title_character_count, sizeof(int));
	for (int i = 0; i < title_character_count; i++) title_file >> character_vertex_count[i];

	b2Vec2** character_vertex = (b2Vec2**)calloc(title_character_count, sizeof(b2Vec2*));
	b2Vec2** character_vertex_moved = (b2Vec2**)calloc(title_character_count, sizeof(b2Vec2*));

	for (int i = 0; i < title_character_count; i++)
	{
		character_vertex[i] = (b2Vec2*)calloc(character_vertex_count[i], sizeof(b2Vec2));
		character_vertex_moved[i] = (b2Vec2*)calloc(character_vertex_count[i], sizeof(b2Vec2));
		for (int j = 0; j < character_vertex_count[i]; j++)
		{
			float xxxx, yyyy;
			title_file >> xxxx >> yyyy;
			character_vertex_moved[i][j] = character_vertex[i][j] = b2Vec2{ xxxx, yyyy };
		}
	}
	//the edge part
	int title_edge_count = -1;
	title_file >> title_edge_count;

	int* character_edge_vertex_count = (int*)calloc(title_edge_count, sizeof(int));
	for (int i = 0; i < title_edge_count; i++) title_file >> character_edge_vertex_count[i];

	b2Vec2** character_edge_vertex = (b2Vec2**)calloc(title_edge_count, sizeof(b2Vec2*));
	b2Vec2** character_edge_vertex_moved = (b2Vec2**)calloc(title_edge_count, sizeof(b2Vec2*));

	for (int i = 0; i < title_edge_count; i++)
	{
		character_edge_vertex[i] = (b2Vec2*)calloc(character_edge_vertex_count[i], sizeof(b2Vec2));
		character_edge_vertex_moved[i] = (b2Vec2*)calloc(character_edge_vertex_count[i], sizeof(b2Vec2));
		for (int j = 0; j < character_edge_vertex_count[i]; j++)
		{
			float xxxx, yyyy;
			title_file >> xxxx >> yyyy;
			character_edge_vertex[i][j] = character_edge_vertex_moved[i][j] = b2Vec2{ xxxx, yyyy };
		}
	}
	title_file.close();

	//b2Color color_of_title_edge{ 1.0f, 0.0f, 0.0f, 1.0f };
	//b2Color color_of_title_fill(0.5f * color_of_title_edge.r, 0.5f * color_of_title_edge.g, 0.5f * color_of_title_edge.b, 0.5f);

	float title_brightness = .5f;
	vector<b2Color> color_of_title_fill_list{};
	//color_of_title_fill_list.reserve(title_character_count);
	for (int i = 0; i < title_character_count; i++) color_of_title_fill_list.push_back(b2Color{ title_brightness + (1.0f - title_brightness)*rand_zero2one(),title_brightness + (1.0f - title_brightness)*rand_zero2one(),title_brightness + (1.0f - title_brightness)*rand_zero2one(),0.7f });

	int RoundCount = 0;

	while (!glfwWindowShouldClose(mainWindow))
	{
		glfwGetWindowSize(mainWindow, &g_camera.m_width, &g_camera.m_height);

		int bufferWidth, bufferHeight;
		glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);
		glViewport(0, 0, bufferWidth, bufferHeight);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplGlfwGL3_NewFrame();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2((float)g_camera.m_width, (float)g_camera.m_height));
		ImGui::Begin("Overlay", NULL, ImVec2(0, 0), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::SetCursorPos(ImVec2(5, (float)g_camera.m_height - 20));
		ImGui::Text("%.1f ms", 1000.0 * frameTime);
		ImGui::End();

		if (!test->is_valid())
		{
			if ((!global_settings.SelectMode))	cout.flush();
			evo_main.fitnessl = ((evo_world*)test)->m_result;

			if ((!global_settings.SelectMode) && global_settings.create_log && (!CreateLog(evo_main, evo_main.step))) return 1;

			//new terrain
			if (global_settings.always_switch_terrain) global_settings.switch_terrain = true;
			if (global_settings.switch_terrain || global_settings.SelectMode)
			{
				global_settings.switch_terrain = false;
				TerrainRefresh();
				//((evo_world*)test)->create_terrain();
				flags = get_flag_position(FetchTerrain(), terrain_xmax, global_settings.how_to_judge);
			}

			if (!global_settings.SelectMode)
			{
				evo_main.next_gen();
				if (global_settings.create_log) FitnessLog(evo_main, evo_main.step - 1);
			}
			sRestart(evo_main.gene_pool);
			RoundCount = 0;
		}

		if (global_settings.needToLoadMainArena)
		{
			global_settings.needToLoadMainArena = false;
			if (global_settings.foundImport)
			{
				initCar.open(global_settings.importFileName);
				if (!initCar)
				{
					cout << "fail to open initial car file\n";
					global_settings.foundImport = false;
					initGene = rand_gene(gene_count);
				}
				else initCar >> initGene;
				initCar.close();
			}
			else initGene = rand_gene(gene_count);

			evo_main = evolution{ initGene };
			SetTerrainMode(global_settings.how_to_judge);
			delete test;
			test = new evo_world{ evo_main.gene_pool ,global_settings.how_to_judge };
			//((evo_world*)test)->create_terrain();
			flags = get_flag_position(FetchTerrain(), terrain_xmax, global_settings.how_to_judge);
			if (global_settings.create_log)
			{
				if (!TerrainLog(FetchTerrain())) return 1;
			}
			RoundCount = 0;
		}

		//gravity setting
		if (global_settings.gravity_storm_on&&rand_zero2one() < .02) global_settings.gravity = rand_zero2one()*25.0 - 5.0;
		((evo_world*)test)->set_gravity(global_settings.gravity);

		sSimulate();
		if (global_settings.show_dynamic)
		{
			sInterface();

			//draw flags
			if (global_settings.show_flags)
			{
				for (b2Vec2 f : flags)
				{
					float stupid_height = g_camera.m_height*g_camera.m_zoom / 10.0f;

					for (int i = 0; i < 5; i++) flag_shape_temp[i] = f + b2Vec2{ 0.0f, stupid_height*0.10f }+b2Vec2{ flag_shape[i].x * stupid_height,flag_shape[i].y *stupid_height };

					g_debugDraw.DrawSegment(f, f + b2Vec2{ 0.0f,stupid_height*0.10f }, color_of_flags);
					g_debugDraw.DrawSolidPolygon(flag_shape_temp, 5, color_of_flags);
					
					char xxxxx[10];
					sprintf_s(xxxxx, "%.0f", f.x);
					g_debugDraw.DrawString_black(f + b2Vec2{ stupid_height*0.015f,stupid_height*0.120f }, xxxxx);
				}
			}


			//draw the title
			if (global_settings.SelectMode) // draw title
			{
				//b2Vec2 bbbbb[4] = { b2Vec2{0,0}+g_camera.m_center,b2Vec2{ 10,0 }+g_camera.m_center,b2Vec2{ 10,10 }+g_camera.m_center,b2Vec2{ 0,10 }+g_camera.m_center };
				//g_debugDraw.DrawSolidPolygon(bbbbb, 4, color_of_title);
				for (int p = 0; p < title_character_count; p++)
				{
					for (int q = 0; q < character_vertex_count[p]; q++)
					{
						character_vertex_moved[p][q] = character_vertex[p][q] + g_camera.m_center;
						g_debugDraw.DrawSolidPolygon_noedge(character_vertex_moved[p], character_vertex_count[p], color_of_title_fill_list.at(p));
					}
				}
				//for (int p = 0; p < title_edge_count; p++)
				//{
				//	for (int q = 0; q < character_edge_vertex_count[p]; q++)
				//	{
				//		character_edge_vertex_moved[p][q] = character_edge_vertex[p][q] + g_camera.m_center;
				//		g_debugDraw.DrawPolygon(character_edge_vertex_moved[p], character_edge_vertex_count[p], color_of_title_edge);
				//	}
				//}
			}
		}

		if (global_settings.SelectMode) sSelection();

		if ((!global_settings.SelectMode) && (!global_settings.needToLoadMainArena))
		{
			if (RoundCount != 0) cout.flush();
			int alive_count = 0;
			for (bool cari : ((evo_world*)test)->m_is_alive)
				if (cari) alive_count++;
			cout << alive_count << " robots alive @ round " << RoundCount << " of batch " << evo_main.step << "\r";
			++RoundCount;
		}

		if (global_settings.show_dynamic)
		{
			// Measure speed
			double time2 = glfwGetTime();
			double alpha = 0.9f;
			frameTime = alpha * frameTime + (1.0 - alpha) * (time2 - time1);
			time1 = time2;

			ImGui::Render();
			glfwSwapBuffers(mainWindow);
		}
		
		glfwPollEvents();
	}

	if (test)
	{
		delete test;
		test = NULL;
	}

	g_debugDraw.Destroy();
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}
