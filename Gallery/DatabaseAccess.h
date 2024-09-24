#pragma once
#include <list>
#include "Album.h"
#include "User.h"
#include "IDataAccess.h"
#include "sqlite3.h"
#include "io.h"

class DatabaseAccess : public IDataAccess
{
public:
	DatabaseAccess();
	virtual ~DatabaseAccess();

	// album related
	const std::list<Album> getAlbums() override;
	const std::list<Album> getAlbumsOfUser(const User& user) override;
	void createAlbum(const Album& album) override;
	void deleteAlbum(const std::string& albumName, int userId) override;
	bool doesAlbumExists(const std::string& albumName, int userId) override;
	Album openAlbum(const std::string& albumName) override;
	void closeAlbum(Album& pAlbum) override;
	void printAlbums() override;

	// picture related
	void addPictureToAlbumByName(const std::string& albumName, Picture& picture) override;
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;

	// user related
	void printUsers() override;
	void createUser(User& user) override;
	void deleteUser(const User& user) override;
	bool doesUserExists(int userId) override;
	User getUser(int userId) override;
	std::list<User> getUsers() override;

	// user statistics
	int countAlbumsOwnedOfUser(const User& user) override;
	int countAlbumsTaggedOfUser(const User& user) override;
	int countTagsOfUser(const User& user) override;
	float averageTagsPerAlbumOfUser(const User& user) override;

	// queries
	User getTopTaggedUser() override;
	Picture getTopTaggedPicture() override;
	std::list<Picture> getTaggedPicturesOfUser(const User& user) override;

	bool open() override;
	void close() override;
	void clear() override;

private:
	sqlite3* db;
	std::string dbFileName = "MyDB.sqlite";;

	std::pair<bool, std::pair<int, Album>> getAlbumIfExists(const std::string& albumName);
	std::list<Picture> getAllPicturesOfAlbum(int albumId);
	std::set<int> getUsersTaggedInPic(int PicId);
	Picture getPictureById(int PicId);
	bool checkIfPictureExists(int albumId, Picture& picture);
	Album createDummyAlbum(const User& user);
	void cleanUserData(const User& userId);
};

