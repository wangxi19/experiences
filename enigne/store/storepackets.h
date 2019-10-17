#ifndef STOREPACKETS_H
#define STOREPACKETS_H

#include <string>

class StorePackets
{
public:
    explicit StorePackets();
    ~StorePackets();
    bool init(const std::string& iCharacDetcStorFolder,const std::string& iEncryptStorFolder,const std::string& iAPT29StorFolder);

    bool setCharacDetcStorFolder(const std::string &characDetcStorFolder);
    bool setEncryptStorFolder(const std::string &encryptStorFolder);
    bool setAPT29StorFolder(const std::string &aPT29StorFolder);

    static bool CreateDirIfNotExists(const std::string& folder);

public:
    std::string mCharacDetcStorFolder;
    std::string mEncryptStorFolder;
    std::string mAPT29StorFolder;
};

#endif // STOREPACKETS_H
