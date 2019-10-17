#include "storepackets.h"
#include "../logtool/logtool.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

StorePackets::StorePackets()
{

}

StorePackets::~StorePackets()
{

}

bool StorePackets::init(const std::string &iCharacDetcStorFolder, const std::string &iEncryptStorFolder, const std::string &iAPT29StorFolder)
{
    if (!setAPT29StorFolder(iAPT29StorFolder)) return false;

    if (!setEncryptStorFolder(iEncryptStorFolder)) return false;

    if (!setCharacDetcStorFolder(iCharacDetcStorFolder)) return false;

    return true;
}

bool StorePackets::CreateDirIfNotExists(const std::string &folder)
{
    if (0 == access(folder.c_str(), F_OK)) {
        if (0 == access(folder.c_str(), W_OK)) {
            return true;
        }

        if (0 == chmod(folder.c_str(), 0777)) {
            return true;
        } else {
            LOGERROR("Fail to execute chmod on folder %s; errno: %d\n", folder.c_str(), errno);
            return false;
        }
    }

    if (0 != mkdir(folder.c_str(), 0777)) {
        LOGERROR("Fail to execute mkdir 0777 on folder %s; errno: %d\n", folder.c_str(), errno);
        return false;
    }

    return true;
}

bool StorePackets::setAPT29StorFolder(const std::string &aPT29StorFolder)
{
    mAPT29StorFolder = aPT29StorFolder;

    if (!CreateDirIfNotExists(mAPT29StorFolder)) {
        mAPT29StorFolder.clear();
        return false;
    }
    return true;
}

bool StorePackets::setEncryptStorFolder(const std::string &encryptStorFolder)
{
    mEncryptStorFolder = encryptStorFolder;

    if (!CreateDirIfNotExists(mEncryptStorFolder)) {
        mEncryptStorFolder.clear();
        return false;
    }

    return true;
}

bool StorePackets::setCharacDetcStorFolder(const std::string &characDetcStorFolder)
{
    mCharacDetcStorFolder = characDetcStorFolder;

    if (!CreateDirIfNotExists(mCharacDetcStorFolder)) {
        mCharacDetcStorFolder.clear();
        return false;
    }

    return true;
}
