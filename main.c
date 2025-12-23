#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <conio.h>
#include <windows.h>
#include <urlmon.h>

// CONFIGURATION
#define OPENASAR_URL "https://github.com/GooseMod/OpenAsar/releases/download/nightly/app.asar"

// Definitions for Windows Keys & Arrows & Colors
#define KEY_UP    72
#define KEY_DOWN  80
#define KEY_ENTER 13
#define KEY_Q     113

#define ARROW_UP   "\xe2\x86\x91"
#define ARROW_DOWN "\xe2\x86\x93"

#define CYAN  "\x1b[36m"
#define GREEN "\x1b[32m"
#define BLUE  "\x1b[34m"
#define RESET "\x1b[0m"
#define BOLD  "\x1b[1m"
#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// LOGIC STRUCTS & GLOBALS

typedef struct {
    const char *name;
    const char *folder;
} DiscordBuild;

// Private list of builds to check
static const DiscordBuild DISCORD_BUILDS[] = {
  { "Stable", "Discord" },
  { "PTB",    "DiscordPTB" },
  { "Canary", "DiscordCanary" },
  { "Dev",    "DiscordDevelopment" },
  { NULL, NULL }
};

// HELPER FUNCTIONS

// 1 = exists, 0 = non exist
static int folderExists(const char *path) {
  return _access(path, 0) == 0;
}

// 1 if file is small (OpenAsar), 0 if large (Stock), -1 on error
int isOpenAsar(const char *path){
  FILE *f = fopen(path, "rb");
  if(f == NULL) return -1;
  int oneMb = 1048576;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fclose(f);

  if(size < oneMb){
    return 1;
  }
  return 0;
}

// 1 exists, 0 non exist
int findAsarBak(const char *asarPath) {
  char bakPath[MAX_PATH] = "\0";
  snprintf(bakPath, MAX_PATH, "%s.bak", asarPath);

  if(folderExists(bakPath) == 1){
    return 1;
  }
  return 0;
}

// CORE LOGIC FUNCTIONS

// Returns index of build found, or -1 if none
int findDiscord(char *asarDest, size_t destSize){
  const char *localData = getenv("LOCALAPPDATA");
  if(localData == NULL) { printf("localappdata does not exist"); return -1; }
  
  const char asarLoc[MAX_PATH] = "resources\\app.asar";
  char discordPath[MAX_PATH] = "\0";
  char appPath[512] = "\0";
  char asarPath[1024] = "\0";
  int i = 0;

  for(i = 0; DISCORD_BUILDS[i].name != NULL; i++){
    snprintf(discordPath, sizeof(discordPath), "%s\\%s", localData, DISCORD_BUILDS[i].folder);
    if(folderExists(discordPath) == 1){
      break;
    }
  }

  // Check if loop finished without finding anything
  if(DISCORD_BUILDS[i].name == NULL){
    return -1;
  }

  snprintf(appPath, sizeof(appPath), "%s\\app-*", discordPath);

  WIN32_FIND_DATA find_data;
  HANDLE h_find = FindFirstFile(appPath, &find_data);

  if (h_find == INVALID_HANDLE_VALUE){
    return -1;
  }

  snprintf(asarPath, sizeof(asarPath), "%s\\%s\\%s", discordPath, find_data.cFileName, asarLoc);
  strncpy(asarDest, asarPath, destSize);
  asarDest[destSize - 1] = '\0';
  
  FindClose(h_find);  
  return i;
}

int getBuildName(char *build, int buildNum, size_t buildSize){
  if(buildNum != -1){
    snprintf(build, buildSize, "%s", DISCORD_BUILDS[buildNum].name);
    return 1;
  }
  return 0;
}

int installOpenAsar(const char *path){
  char backupPath[MAX_PATH] = "\0";

  snprintf(backupPath, MAX_PATH, "%s.bak", path);

  int backupExist = findAsarBak(path);

  if(backupExist == 1){
    printf("Backup already found. Skipping backup creation.\n");
  } else{
    printf("Creating backup...\n");
    if(rename(path, backupPath) != 0){
      printf("Error: Could not rename file. Is Discord running?\n");
      return -1;
    }
    printf("Backup successfully created!\n");
  }

  if(isOpenAsar(path) == 1){
    printf("OpenAsar already exists.\n"); 
    return 1;
  }

  printf("Downloading OpenAsar...\n");
  HRESULT hr = URLDownloadToFileA(NULL, OPENASAR_URL, path, 0, NULL);

  if (hr == S_OK) {
    printf("Successfully installed OpenAsar!\n");
    return 1;
  } else {
    printf("Error: Download failed (Code: 0x%lx).\n", (unsigned long)hr);
    printf("Restoring backup...\n");
    rename(backupPath, path);
    return 0;
  }
}

int uninstallOpenAsar(const char *path) {
  char backupPath[MAX_PATH];
  snprintf(backupPath, MAX_PATH, "%s.bak", path);

  if(folderExists(backupPath) == 0){
    printf("Error: No backup found (%s). Cannot uninstall.\n", backupPath);
    return -1;
  }

  if(isOpenAsar(path) == 0){
    printf("OpenAsar does not exist. Cannot uninstall.\n"); 
    return 0;
  }

  if(remove(path) != 0){
    printf("Error: Could not delete current file. Is Discord running?\n");
    return -1;
  }

  if(rename(backupPath, path) != 0){
    printf("Critical Error: Deleted OpenAsar but could not restore backup.\n");
    return -1;
  }

  printf("Uninstalled successfully (Restored Stock Discord).\n");
  return 1;
}

// TUI HELPER FUNCTIONS

int getKeyInput(void){
  int key = _getch();
  switch(key){
    case 0:
    case 224: { return _getch(); }
    default: { return key; }
  }
  return 0;
}

void clrscr(){
    printf("\x1b[2J\x1b[3J\x1b[H");
}

void renderMenu(const char **options, int count, int selection){
  for(int i = 0; i < count; i++){
    if(i == selection){
      printf("%s%s> %s%s\x1b[K\n", CYAN, BOLD, options[i], RESET);
    } else{
      printf("  %s\x1b[K\n", options[i]);
    }
  }
}

void setupWindowsConsole(){
  SetConsoleOutputCP(65001);
  HANDLE handleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(handleOutput, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(handleOutput, dwMode);
}

// MENU ACTIONS

void installAsar(){
  clrscr();
  char buildName[32] = "\0";
  char asarPath[MAX_PATH] = "\0";

  int result = findDiscord(asarPath, MAX_PATH);

  if (result != -1) {
    getBuildName(buildName, result, sizeof(buildName));
    printf("%s%s%s - %s\n\n", BLUE, buildName, RESET, asarPath);
    
    installOpenAsar(asarPath);
  } else {
    printf("No Discord installation found.\n");
  }

  printf("\nPress any key to return...");
  _getch();
}

void uninstallAsar(){
  clrscr();
  char buildName[32] = "\0";
  char asarPath[MAX_PATH] = "\0";

  printf("Searching for Discord to Uninstall...\n");
  int result = findDiscord(asarPath, MAX_PATH);

  if (result != -1) {
      getBuildName(buildName, result, sizeof(buildName));
      printf("%s%s%s - %s\n\n", BLUE, buildName, RESET, asarPath);

      uninstallOpenAsar(asarPath);
  } else {
      printf("No Discord installation found.\n");
  }

  printf("\nPress any key to return...");
  _getch();
}

// MAIN ENTRY POINT

int main(void){
  setupWindowsConsole();
  clrscr();

  const char *options[] = {
    "Install OpenAsar",
    "Uninstall OpenAsar"
  };
  int option_count = sizeof(options) / sizeof(options[0]);
  int current_selection = 0;
  int running = 1;

  printf("For navigation (%s %s) To quit (Q)\n", ARROW_DOWN, ARROW_UP);
  printf("%s?%s What would you like to do? (Press Enter to confirm):\n", GREEN, RESET);
  printf(HIDE_CURSOR);

  while(running){
    renderMenu(options, option_count, current_selection);

    switch(getKeyInput()){
      case KEY_UP:
        current_selection--;
        if (current_selection < 0) current_selection = option_count - 1;
        break;
      case KEY_DOWN:
        current_selection++;
        if (current_selection >= option_count) current_selection = 0;
        break;
      case KEY_ENTER:
        running = 0;
        break;
      case KEY_Q:
        printf(SHOW_CURSOR);
        printf("\nGoodbye.\n");
        return 0;
    }
    
    printf("\033[%dA", option_count);
  }

  printf(SHOW_CURSOR);
  printf("\n");

  switch(current_selection){
    case 0:
      installAsar();
      break;
    case 1:
      uninstallAsar();
      break;
    default:
      printf("ERROR: Unknown option!");
      return -1;
  }

  return 0;

}
