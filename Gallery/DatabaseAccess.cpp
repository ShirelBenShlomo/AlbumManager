#include "DatabaseAccess.h"
#include "ItemNotFoundException.h"

bool DatabaseAccess::open()
{
    int doesFileExist = _access(this->dbFileName.c_str(), 0);
	int res = sqlite3_open(this->dbFileName.c_str(), &this->db);
	if (res != SQLITE_OK) {
        db = nullptr;
        std::cout << "Failed to open DB" << std::endl;
        return false;
	}

    if (doesFileExist != 0) {
        // Build the tables if the file doesn't exist
        const char* createUserTableSQL = "CREATE TABLE IF NOT EXISTS USERS ("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "NAME TEXT NOT NULL);";

        const char* createAlbumTableSQL = "CREATE TABLE IF NOT EXISTS ALBUMS ("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "NAME TEXT NOT NULL,"
            "CREATION_DATE TEXT NOT NULL,"
            "USER_ID INTEGER NOT NULL,"
            "FOREIGN KEY(USER_ID) REFERENCES USERS(ID));";

        const char* createTagTableSQL = "CREATE TABLE IF NOT EXISTS TAGS ("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "PICTURE_ID INTEGER NOT NULL,"
            "USER_ID INTEGER NOT NULL,"
            "FOREIGN KEY(PICTURE_ID) REFERENCES PICTURES(ID),"
            "FOREIGN KEY(USER_ID) REFERENCES USERS(ID));";

        const char* createPictureTableSQL = "CREATE TABLE IF NOT EXISTS PICTURES ("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "NAME TEXT NOT NULL,"
            "LOCATION TEXT NOT NULL,"
            "CREATION_DATE TEXT NOT NULL,"
            "ALBUM_ID INTEGER NOT NULL,"
            "FOREIGN KEY(ALBUM_ID) REFERENCES ALBUMS(ID));";

        char* errMessage = nullptr;
        res = sqlite3_exec(db, createUserTableSQL, nullptr, nullptr, &errMessage);
        if (res != SQLITE_OK) {
            std::cerr << "Error creating USERS table: " << errMessage << std::endl;
            sqlite3_free(errMessage);
            return false;
        }

        res = sqlite3_exec(db, createAlbumTableSQL, nullptr, nullptr, &errMessage);
        if (res != SQLITE_OK) {
            std::cerr << "Error creating ALBUMS table: " << errMessage << std::endl;
            sqlite3_free(errMessage);
            return false;
        }

        res = sqlite3_exec(db, createTagTableSQL, nullptr, nullptr, &errMessage);
        if (res != SQLITE_OK) {
            std::cerr << "Error creating TAGS table: " << errMessage << std::endl;
            sqlite3_free(errMessage);
            return false;
        }

        res = sqlite3_exec(db, createPictureTableSQL, nullptr, nullptr, &errMessage);
        if (res != SQLITE_OK) {
            std::cerr << "Error creating PICTURES table: " << errMessage << std::endl;
            sqlite3_free(errMessage);
            return false;
        }
    }

	return true;
}

void DatabaseAccess::close()
{
	sqlite3_close(this->db);
	this->db = nullptr;
}

void DatabaseAccess::clear()
{
	close();
}

std::pair<bool, std::pair<int, Album>> DatabaseAccess::getAlbumIfExists(const std::string& albumName)
{
    std::string query = "SELECT * FROM ALBUMS WHERE NAME = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw MyException("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, albumName.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::make_pair(false, std::make_pair(-1, Album()));
    }

    int albumId = sqlite3_column_int(stmt, 0);
    std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    std::string creationDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    int userId = sqlite3_column_int(stmt, 3);

    sqlite3_finalize(stmt);

    Album album = Album(userId, name, creationDate);

    std::list<Picture> pictures = getAllPicturesOfAlbum(albumId);
    for (auto picture : pictures) {
        album.addPicture(picture);
    }
    
    return std::make_pair(true, std::make_pair(albumId, album));
}

std::list<Picture> DatabaseAccess::getAllPicturesOfAlbum(int albumId)
{
    std::list<Picture> pictures;

    std::string query = "SELECT * FROM PICTURES WHERE ALBUM_ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, albumId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string creationDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int userId = sqlite3_column_int(stmt, 2);

        Picture picture(id, name, location, creationDate);

        std::set<int> usersTagged = getUsersTaggedInPic(id);
        for (auto userId : usersTagged) {
            picture.tagUser(userId);
        }

        pictures.emplace_back(picture);
    }

    sqlite3_finalize(stmt);
    return pictures;
}

std::set<int> DatabaseAccess::getUsersTaggedInPic(int PicId)
{
    std::set<int> tags;

    std::string query = "SELECT USER_ID FROM TAGS WHERE PICTURE_ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, PicId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);

        tags.insert(userId);
    }

    sqlite3_finalize(stmt);
    return tags;
}

Picture DatabaseAccess::getPictureById(int PicId)
{
    std::string query = "SELECT * FROM PICTURES WHERE ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, PicId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string creationDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        Picture picture(id, name, location, creationDate);

        std::set<int> usersTagged = getUsersTaggedInPic(id);
        for (auto userId : usersTagged) {
            picture.tagUser(userId);
        }

        sqlite3_finalize(stmt);

        return picture;
    }
    else {
        throw MyException("Picture not found");
    }
}

bool DatabaseAccess::checkIfPictureExists(int albumId, Picture& picture)
{
    std::string query = "SELECT * FROM PICTURES WHERE ALBUM_ID = ? AND NAME = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw MyException("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, albumId);
    sqlite3_bind_text(stmt, 2, picture.getName().c_str(), -1, SQLITE_STATIC);

    bool pictureExists = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pictureExists = true;
    }

    sqlite3_finalize(stmt);

    return pictureExists;
}

DatabaseAccess::DatabaseAccess()
{
    open();
}

DatabaseAccess::~DatabaseAccess()
{
    clear();
}

const std::list<Album> DatabaseAccess::getAlbums()
{
    std::list<Album> albums;

    std::string query = "SELECT * FROM ALBUMS";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int albumId = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string creationTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int userId = sqlite3_column_int(stmt, 3);

        Album album(userId, name, creationTime);

        std::list<Picture> pictures = getAllPicturesOfAlbum(albumId);
        for (auto picture : pictures) {
            album.addPicture(picture);
        }

        albums.emplace_back(album);
    }

    sqlite3_finalize(stmt);

    return albums;
}


const std::list<Album> DatabaseAccess::getAlbumsOfUser(const User& user)
{
    std::list<Album> albums;

    std::string query = "SELECT * FROM ALBUMS WHERE USER_ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, user.getId());

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int albumId = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string creationTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int userId = sqlite3_column_int(stmt, 3);
        
        Album album(userId, name, creationTime);

        std::list<Picture> pictures = getAllPicturesOfAlbum(albumId);
        for (auto picture : pictures) {
            album.addPicture(picture);
        }

        albums.emplace_back(album);
    }

    sqlite3_finalize(stmt);

    return albums;
}

void DatabaseAccess::createAlbum(const Album& album)
{
    std::pair<bool, std::pair<int, Album>> albumInfo = getAlbumIfExists(album.getName());
    if (albumInfo.first) {
        throw MyException("Error: Album with name " + album.getName() + " already exists");
    }

    std::string insertTagQuery = "INSERT INTO ALBUMS (NAME, CREATION_DATE, USER_ID) VALUES (?, ?, ?)";
    sqlite3_stmt* insertTagStmt;
    if (sqlite3_prepare_v2(db, insertTagQuery.c_str(), -1, &insertTagStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(insertTagStmt, 1, album.getName().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insertTagStmt, 2, album.getCreationDate().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(insertTagStmt, 3, album.getOwnerId());

    if (sqlite3_step(insertTagStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to prepare statement " + std::string(sqlite3_errmsg(db)));
    }
    else {
        //std::cout << "User tagged in picture successfully\n";
    }

    sqlite3_finalize(insertTagStmt);
}

void DatabaseAccess::deleteAlbum(const std::string& albumName, int userId)
{
    std::string query = "DELETE FROM ALBUMS WHERE USER_ID = ? AND NAME = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, albumName.c_str(), -1, SQLITE_STATIC); // Bind albumName parameter

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement: " + std::string(sqlite3_errmsg(db)));
    }
    else {
        //std::cout << "Album deleted successfully\n";
    }

    sqlite3_finalize(stmt);
}

bool DatabaseAccess::doesAlbumExists(const std::string& albumName, int userId)
{
    bool exists = false;

    std::string query = "SELECT COUNT(*) FROM Albums WHERE name = ? AND user_id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    // Bind parameters
    sqlite3_bind_text(stmt, 1, albumName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    // Execute the prepared statement
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        exists = (count > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}

Album DatabaseAccess::openAlbum(const std::string& albumName)
{
    std::pair<bool, std::pair<int, Album>> findAlbum = getAlbumIfExists(albumName);

    if (findAlbum.first) {
        return findAlbum.second.second;
    }
    
    throw MyException("No album with name " + albumName + " exists");
}

void DatabaseAccess::closeAlbum(Album& pAlbum)
{
    // should delete album, there was no allocation so no need for deletion
}

void DatabaseAccess::printAlbums()
{
    std::list<Album> albums = getAlbums();
    if (albums.empty()) {
        throw MyException("There are no existing albums.");
    }
    std::cout << "Album list:" << std::endl;
    std::cout << "-----------" << std::endl;
    for (const Album& album : albums) {
        std::cout << std::setw(5) << "* " << album;
    }
}

void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, Picture& picture)
{
    std::pair<bool, std::pair<int, Album>> findAlbum = getAlbumIfExists(albumName);
    if (!findAlbum.first) {
        throw MyException("There are no existing albums with the entered name.");
    }
    int albumId = findAlbum.second.first;

    if (checkIfPictureExists(albumId, picture)) {
        throw MyException("Picture with the name " + picture.getName() + " already exists.");
    }
    std::string query = "INSERT INTO PICTURES (NAME, LOCATION, CREATION_DATE, ALBUM_ID) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    
    sqlite3_bind_text(stmt, 1, picture.getName().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, picture.getPath().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, picture.getCreationDate().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, albumId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement: " + std::string(sqlite3_errmsg(db)));
    }
    else {
        picture.setId(sqlite3_last_insert_rowid(this->db));
        //std::cout << "User tagged in picture successfully\n";
    }

    sqlite3_finalize(stmt);
}

void DatabaseAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName)
{
    std::pair<bool, std::pair<int, Album>> findAlbum = getAlbumIfExists(albumName);
    if (!findAlbum.first) {
        throw MyException("There are no existing albums with the entered name.");
    }
    int albumID = findAlbum.second.first;

    std::string deleteTagQuery = "DELETE FROM PICTURES WHERE ALBUM_ID = ? AND NAME = ?";
    sqlite3_stmt* deleteTagStmt;
    if (sqlite3_prepare_v2(db, deleteTagQuery.c_str(), -1, &deleteTagStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(deleteTagStmt, 1, albumID);
    sqlite3_bind_text(deleteTagStmt, 2, pictureName.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(deleteTagStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    else {
        //std::cout << "User untagged from picture successfully\n";
    }

    sqlite3_finalize(deleteTagStmt);
}

void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
    // Retrieve picture ID from the Pictures table
    Album albumToTag = openAlbum(albumName);
    Picture picture = albumToTag.getPicture(pictureName);
    int pictureId = picture.getId();
    //closeAlbum(albumToTag);

    // Insert tag into the Tags table
    std::string insertTagQuery = "INSERT INTO TAGS (USER_ID, PICTURE_ID) VALUES (?, ?)";
    sqlite3_stmt* insertTagStmt;
    if (sqlite3_prepare_v2(db, insertTagQuery.c_str(), -1, &insertTagStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(insertTagStmt, 1, userId);
    sqlite3_bind_int(insertTagStmt, 2, pictureId);

    if (sqlite3_step(insertTagStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    else {
       // std::cout << "User tagged in picture successfully\n";
    }

    sqlite3_finalize(insertTagStmt);
}


void DatabaseAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
    Album albumToTag = openAlbum(albumName);
    Picture picture = albumToTag.getPicture(pictureName);
    int pictureId = picture.getId();
    closeAlbum(albumToTag);

    std::string deleteTagQuery = "DELETE FROM TAGS WHERE USER_ID = ? AND PICTURE_ID = ?";
    sqlite3_stmt* deleteTagStmt;
    if (sqlite3_prepare_v2(db, deleteTagQuery.c_str(), -1, &deleteTagStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(deleteTagStmt, 1, userId);
    sqlite3_bind_int(deleteTagStmt, 2, pictureId);

    if (sqlite3_step(deleteTagStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    else {
        //std::cout << "User untagged from picture successfully\n";
    }

    sqlite3_finalize(deleteTagStmt);
}


void DatabaseAccess::printUsers()
{
    std::cout << "Users list:" << std::endl;
    std::cout << "-----------" << std::endl;

    std::string query = "SELECT * FROM USERS";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        User user(id, name);
        std::cout << user << std::endl;
    }

    sqlite3_finalize(stmt);
}

void DatabaseAccess::createUser(User& user)
{
    std::string query = "INSERT INTO USERS ( NAME) VALUES ( ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, user.getName().c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    else {
        user.setId(sqlite3_last_insert_rowid(this->db));
    }

    sqlite3_finalize(stmt);
}

void DatabaseAccess::deleteUser(const User& user)
{
    // Delete pictures
    std::string deletePicturesQuery = "DELETE FROM PICTURES WHERE ALBUM_ID IN (SELECT ID FROM ALBUMS WHERE USER_ID = ?)";
    sqlite3_stmt* pictureStmt;
    if (sqlite3_prepare_v2(db, deletePicturesQuery.c_str(), -1, &pictureStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(pictureStmt, 1, user.getId());
    if (sqlite3_step(pictureStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_finalize(pictureStmt);

    // Delete albums of the user
    std::string deleteAlbumsQuery = "DELETE FROM ALBUMS WHERE USER_ID = ?";
    sqlite3_stmt* albumStmt;
    if (sqlite3_prepare_v2(db, deleteAlbumsQuery.c_str(), -1, &albumStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(albumStmt, 1, user.getId());
    if (sqlite3_step(albumStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_finalize(albumStmt);

    // Delete user
    std::string query = "DELETE FROM USERS WHERE ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(stmt, 1, user.getId());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    else {
        //std::cout << "User deleted successfully\n";
    }   
    
    sqlite3_finalize(stmt);

    // Delete all tags of the user
    std::string deleteTagsQuery = "DELETE FROM TAGS WHERE USER_ID = ?";
    sqlite3_stmt* tagStmt;
    if (sqlite3_prepare_v2(db, deleteTagsQuery.c_str(), -1, &tagStmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(tagStmt, 1, user.getId());
    if (sqlite3_step(tagStmt) != SQLITE_DONE) {
        throw MyException("Error: Failed to execute statement" + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_finalize(tagStmt);

}

bool DatabaseAccess::doesUserExists(int userId)
{
    try {
        getUser(userId);
        return true;
    }catch (const std::exception& e) {
        return false;
    }
}

User DatabaseAccess::getUser(int userId)
{
    std::string query = "SELECT * FROM USERS WHERE ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, userId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        User user(id, name);

        sqlite3_finalize(stmt);

        return user;
    }
    else {
        throw MyException("User not found");
    }
}

std::list<User> DatabaseAccess::getUsers()
{
    std::list<User> users;

    std::string query = "SELECT * FROM USERS";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement " + std::string(sqlite3_errmsg(db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        User user(id, name);
        users.push_back(user);
    }

    sqlite3_finalize(stmt);

    return users;
}

int DatabaseAccess::countAlbumsOwnedOfUser(const User& user)
{
    int count = 0;

    std::string query = "SELECT COUNT(*) FROM ALBUMS WHERE USER_ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, user.getId());

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return count;
}

int DatabaseAccess::countAlbumsTaggedOfUser(const User& user)
{
    int count = 0;

    std::string query = "SELECT COUNT(DISTINCT p.album_id) "
        "FROM Tags t "
        "JOIN Pictures p ON t.picture_id = p.id "
        "WHERE t.user_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, user.getId());

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return count;
}

int DatabaseAccess::countTagsOfUser(const User& user)
{
    int count = 0;

    std::string query = "SELECT COUNT(*) FROM TAGS WHERE USER_ID = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, user.getId());

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return count;
}

float DatabaseAccess::averageTagsPerAlbumOfUser(const User& user)
{
    int albumsTaggedCount = countAlbumsTaggedOfUser(user);

    if (0 == albumsTaggedCount) {
        return 0;
    }

    return static_cast<float>(countTagsOfUser(user)) / albumsTaggedCount;
}

User DatabaseAccess::getTopTaggedUser()
{
    int maxTags = 0;
    int maxUserId;
    std::string query = "SELECT USER_ID, COUNT(*) AS TagCount FROM TAGS "
        "GROUP BY USER_ID ORDER BY TagCount DESC LIMIT 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    // Execute the prepared statement
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);
        int tagCount = sqlite3_column_int(stmt, 1);

        if (tagCount > maxTags) {
            maxTags = tagCount;
            maxUserId = userId;
        }
    }

    sqlite3_finalize(stmt);

    if (maxTags > 0) {
        User topUser = getUser(maxUserId);
        return topUser;
    }
    else {
        throw MyException("No tags!");
    }
}

Picture DatabaseAccess::getTopTaggedPicture()
{
    int maxTags = 0;
    int maxPictureId;
    std::string query = "SELECT PICTURE_ID, COUNT(*) AS TagCount FROM TAGS "
        "GROUP BY PICTURE_ID ORDER BY TagCount DESC LIMIT 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int pictureId = sqlite3_column_int(stmt, 0);
        int tagCount = sqlite3_column_int(stmt, 1);

        if (tagCount > maxTags) {
            maxTags = tagCount;
            maxPictureId = pictureId;
        }
    }

    sqlite3_finalize(stmt);

    if (maxTags > 0) {
        Picture topPicture = getPictureById(maxPictureId);
        return topPicture;
    }
    else {
        throw MyException("No tags!");
    }
}

std::list<Picture> DatabaseAccess::getTaggedPicturesOfUser(const User& user)
{
    std::list<Picture> taggedPictures; // List to store tagged pictures
    std::string query = "SELECT PICTURE_ID FROM TAGS WHERE USER_ID = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw MyException("Error: Failed to prepare statement" + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_int(stmt, 1, user.getId());

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int pictureId = sqlite3_column_int(stmt, 0);

        Picture picture = getPictureById(pictureId); 
        taggedPictures.push_back(picture);
    }

    sqlite3_finalize(stmt);

    if (taggedPictures.empty()) {
        throw MyException("No tagged pictures found for the user!");
    }

    return taggedPictures;
}