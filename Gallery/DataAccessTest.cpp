#include "DataAccessTest.h"
#include <Windows.h>

void DataAccessTest::AddingRecords(DatabaseAccess database)
{
	try {
		User user0(0, "user_0");
		database.createUser(user0);
		Album album0(0, "Album_0");
		database.createAlbum(album0);
		Picture pic0(0, "Pic_1");
		database.addPictureToAlbumByName("Album_0", pic0);
		Picture pic1(1, "Pic_2");
		database.addPictureToAlbumByName("Album_0", pic1);

		User user1(1, "user_1");
		database.createUser(user1);
		Album album1(1, "Album_1");
		database.createAlbum(album1);
		Picture pic2(2, "Pic_1");
		database.addPictureToAlbumByName("Album_1", pic2);
		Picture pic3(3, "Pic_2");
		database.addPictureToAlbumByName("Album_1", pic3);

		User user2(2, "user_2");
		database.createUser(user2);
		Album album2(2, "Album_2");
		database.createAlbum(album2);
		Picture pic4(4, "Pic_1");
		database.addPictureToAlbumByName("Album_2", pic4);
		Picture pic5(5, "Pic_2");
		database.addPictureToAlbumByName("Album_2", pic5);

		
		database.tagUserInPicture("Album_0", "Pic_1", 1);
		database.tagUserInPicture("Album_0", "Pic_1", 2);
		database.tagUserInPicture("Album_0", "Pic_2", 1);
		database.tagUserInPicture("Album_0", "Pic_2", 2);

		database.tagUserInPicture("Album_1", "Pic_1", 0);
		database.tagUserInPicture("Album_1", "Pic_1", 2);
		database.tagUserInPicture("Album_1", "Pic_2", 0);
		database.tagUserInPicture("Album_1", "Pic_2", 2);

		database.tagUserInPicture("Album_2", "Pic_1", 0);
		database.tagUserInPicture("Album_2", "Pic_1", 1);
		database.tagUserInPicture("Album_2", "Pic_2", 0);
		database.tagUserInPicture("Album_2", "Pic_2", 1);
		
	}
	catch (std::exception& e) {
		MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
	}
}

void DataAccessTest::updatingRecords(DatabaseAccess database)
{
	try {
		Picture pic0(0, "My femily");
		database.addPictureToAlbumByName("Album_0", pic0);

		database.removePictureFromAlbumByName("Album_0", "My femily");

		pic0.setName("My family");
		database.addPictureToAlbumByName("Album_0", pic0);
	}
	catch (std::exception& e) {
		MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
	}
}

void DataAccessTest::DeletingRecords(DatabaseAccess database)
{
	try {
		User user0(0, "user_0");
		database.deleteUser(user0);
	}
	catch (std::exception& e) {
		MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
	}
}
