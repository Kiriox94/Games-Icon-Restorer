#include <stdio.h>
#include <dirent.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <switch.h>
#include <iostream>
#include <vector>
#include <system_error>

namespace fs = std::filesystem;
std::error_code ec;

std::string contentPath = "sdmc:/atmosphere/contents";
std::string backupPath = "sdmc:/gameIcons/backup";
std::string iconFilePath = "/icon.jpg";

std::vector<std::string> gamesIds;

int menuId = 0;
int gamesCount = 0;

bool deleteRecursive(const std::string &path) {
    DIR *dir = nullptr;
    struct dirent *entry = nullptr;
    dir = opendir(path.c_str());

    if (dir) {
        while((entry = readdir(dir))) {
            std::string filename = entry->d_name;
            if ((filename.compare(".") == 0) || (filename.compare("..") == 0))
                continue;

            std::string file_path = path;
            file_path.append(path.compare("/") == 0? "" : "/");
            file_path.append(filename);

            if (entry->d_type & DT_DIR) {
                deleteRecursive(file_path);
            }
            else {
                if (remove(file_path.c_str()) != 0) {
                    return false;
                }
            }
        }

        closedir(dir);
    }
    else {
        return false;
    }

    return (rmdir(path.c_str()) == 0);
}

bool cp(const char *filein, const char *fileout) {
    FILE *exein, *exeout;
    exein = fopen(filein, "rb");
    if (exein == NULL) {
        /* handle error */
        perror("file open for reading");
        return false;
    }
    exeout = fopen(fileout, "wb");
    if (exeout == NULL) {
        /* handle error */
        perror("file open for writing");
        return false;
    }
    size_t n, m;
    unsigned char buff[131072];
    do {
        n = fread(buff, 1, sizeof buff, exein);
        if (n) m = fwrite(buff, 1, n, exeout);
        else   m = 0;
    }
    while ((n > 0) && (n == m));
    if (m) {
        perror("copy");
        return false;
    }
    if (fclose(exeout)) {
        perror("close output file");
        return false;
    }
    if (fclose(exein)) {
        perror("close input file");
        return false;
    }
    return true;
}

void updateGamesList(std::string folderPath) {
    gamesIds.clear();
    if(fs::exists(folderPath))
        for (const auto & entry : fs::directory_iterator(folderPath))
            if(fs::exists(entry.path().string() + iconFilePath))
                gamesIds.push_back(entry.path().filename().string());
}

int copyIcons(std::string in, std::string out) {
    if(!fs::exists(out)) fs::create_directory(out);
    updateGamesList(in);
    for (const std::string& gameId : gamesIds) {
        if(!fs::exists(out + "/" + gameId)) fs::create_directory(out + "/" + gameId);
        cp((in + "/" + gameId + iconFilePath).c_str(), (out + "/" + gameId + iconFilePath).c_str());
    }
    return gamesIds.size();
}

int main(int argc, char **argv)
{
    consoleInit(NULL);
    socketInitializeDefault();              // Initialize sockets

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    if(!fs::exists("sdmc:/gameIcons")) fs::create_directory("sdmc:/gameIcons");
    updateGamesList(backupPath);

    while(appletMainLoop())
    {
        consoleClear();
        printf("    Games Icons Restorer\n");
        printf("----------------------------\n\n");
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;
        
        if(menuId == 0) {

            printf(" Press A to make backup\n");
            if(gamesIds.size() > 0) printf(" Press Y to restore backup\n");
            if(gamesIds.size() > 0) printf(" Press X to delete backup\n");
            printf(" Press + to exit\n");

            if (kDown & HidNpadButton_A) {
                gamesCount = copyIcons(contentPath, backupPath);
                menuId = 1;
            }
            else if (kDown & HidNpadButton_Y && gamesIds.size() > 0) {
                gamesCount = copyIcons(backupPath, contentPath);
                menuId = 2;
            }else if (kDown & HidNpadButton_X && gamesIds.size() > 0) {
                updateGamesList(backupPath);
                menuId = 3;
            }
        }else if(menuId == 1) {
            printf(" %d icons have been successfully saved\n", gamesCount);
            printf(" Press A to go back\n");

            if (kDown & HidNpadButton_A) {
                menuId = 0;
            }
        }else if(menuId == 2) {
            printf(" %d icons have been successfully restored\n", gamesCount);
            printf(" Do you want to delete your backup ?\n\n");
            printf(" Press A for Yes\n");
            printf(" Press B for no\n");

            if (kDown & HidNpadButton_A) {
                deleteRecursive(backupPath);
                updateGamesList(backupPath);
                menuId = 4;
            }else if (kDown & HidNpadButton_B) {
                updateGamesList(backupPath);
                menuId = 0;
            }
        }else if(menuId == 3) {
            if(gamesIds.size() > 0) {
                printf(" The backup of your icons is about to be deleted, are you sure you want to continue?\n\n");
                printf(" Press A to continue\n");
                printf(" Press B to go back\n");

                if (kDown & HidNpadButton_A) {
                    deleteRecursive(backupPath);
                    updateGamesList(backupPath);
                    menuId = 4;
                }else if (kDown & HidNpadButton_B) {
                    updateGamesList(backupPath);
                    menuId = 0;
                }
            }else {
                menuId = 0;
            }
        }else if(menuId == 4) {
            printf(" Backup have been successfully deleted\n\n");
            printf(" Press A to go back\n");
            
            if (kDown & HidNpadButton_A) {
                menuId = 0;
            }
        }else {
            printf(" Error 404: You are lost\n\n");
            printf(" Press A to go back\n");
            
            if (kDown & HidNpadButton_A) {
                menuId = 0;
            }
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    socketExit();
    return 0;
}