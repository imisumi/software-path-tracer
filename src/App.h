#pragma once

#include <imgui.h>

struct GLFWwindow;

class App
{
public:
	App();
	~App();

	void run();

private:
	static App *s_Instance;

	GLFWwindow *m_window = nullptr;

	bool m_show_demo_window = true;
	bool m_show_another_window = false;
	ImVec4 m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};