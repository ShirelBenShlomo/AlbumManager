/*
#include <iostream>
#include <string>
#include <ctime>
#include "DataBaseAccess.h"
#include "AlbumManager.h"
#include "DataAccessTest.h"

int getCommandNumberFromUser()
{
	std::string message("\nPlease enter any command(use number): ");
	std::string numericStr("0123456789");

	std::cout << message << std::endl;
	std::string input;
	std::getline(std::cin, input);

	while (std::cin.fail() || std::cin.eof() || input.find_first_not_of(numericStr) != std::string::npos) {

		std::cout << "Please enter a number only!" << std::endl;

		if (input.find_first_not_of(numericStr) == std::string::npos) {
			std::cin.clear();
		}

		std::cout << std::endl << message << std::endl;
		std::getline(std::cin, input);
	}

	return std::atoi(input.c_str());
}

void printSystemInfo() {
	std::cout << "Shirel Ben Shlomo gallery proj" << std::endl;
	std::time_t currentTime = std::time(0);
	std::string timeString = std::ctime(&currentTime);
	std::cout << "Current time and date: " << timeString << std::endl;
}

int main(void)
{
	// initialization data access
	DatabaseAccess dataAccess;

	// initialize album manager
	AlbumManager albumManager(dataAccess);

	DataAccessTest::AddingRecords(dataAccess);
	DataAccessTest::updatingRecords(dataAccess);
	//DataAccessTest::DeletingRecords(dataAccess);

	std::string albumName;
	std::cout << "Welcome to Gallery!" << std::endl;
	printSystemInfo();
	std::cout << "Type " << HELP << " to a list of all supported commands" << std::endl;

	do {
		int commandNumber = getCommandNumberFromUser();

		try {
			albumManager.executeCommand(static_cast<CommandType>(commandNumber));
		}
		catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	} while (true);
}
*/
#include <windows.h>
#include <string>
#include "GuiAlbumManager.h"
#include "DataBaseAccess.h"
#include "DataAccessTest.h"

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	DatabaseAccess dataAccess;

	GuiAlbumManager albumManager(dataAccess);
	
	//DataAccessTest::AddingRecords(dataAccess);
	//DataAccessTest::updatingRecords(dataAccess);
	//DataAccessTest::DeletingRecords(dataAccess);

	try {
		albumManager.CreateMainWindow();
	}
	catch (std::exception& e) {
		MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
	}

	

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}