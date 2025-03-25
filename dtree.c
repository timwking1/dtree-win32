/*=============================================================================
*       dtree.c
*       timwking1
*       20-Mar 2025
=============================================================================*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")

/*=============================================================================
*   Constants
=============================================================================*/

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TREEVIEW_WIDTH 250
#define MAX_LOADSTRING 255

#define IDM_NEW 101
#define IDM_OPEN 102
#define IDM_SAVE 103
#define IDM_EXIT 104
#define IDM_ABOUT 105

#define ID_TREEVIEW 201
#define ID_EDIT_NAME 202
#define ID_EDIT_DESCRIPTION 203
#define ID_SAVE_ITEM 204

/*=============================================================================
*   Global Declarations
*       Indicated by "g_" leading the field name
=============================================================================*/

HINSTANCE g_hInstance;
HWND g_hWnd;
HWND g_hTreeView;
HWND g_hEditName;
HWND g_hEditDesc;
HWND g_hSaveButton;
HTREEITEM g_hSelectedItem;
char g_szFileName[MAX_PATH] = "";

/*=============================================================================
*   Struct Definitions
=============================================================================*/

typedef struct _TreeNodeData 
{
    char name[MAX_LOADSTRING];
    char description[MAX_LOADSTRING];
} TreeNodeData;

/*=============================================================================
*   Declarations
=============================================================================*/

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void InitializeUI(HWND hwnd);
void AddItemToTree(HWND hTreeView, HTREEITEM hParent, const char* textm, TreeNodeData* data);
void SaveTreeToFile(HWND hTreeView, const char* fileName);
void LoadTreeFromFile(HWND hTreeView, const char* fileName);
void UpdateEditFields(HWND hTreeView, HTREEITEM hItem);
void SaveItemChanges(HWND hTreeView, HTREEITEM hItem);
HTREEITEM RecursiveSaveTree(HWND hTreeView, HTREEITEM hItem, FILE* file, int level);
HTREEITEM RecursiveLoadTree(HWND hTreeView, HTREEITEM hParent, FILE* file, int level);
void CreateNewItem(HWND hTreeView, HTREEITEM hParent);

/*=============================================================================
*   WinMain 
*       The entry point of a win32 application
*       ***gcc REQUIRES the -mwindows flag to recognize this as the entry point!***
=============================================================================*/
int FAR PASCAL WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevious, LPSTR lpCmdLine, int nCmdShow)
{
    /*
    *   Initialize the instance handle reference and the common controls
    *   used in the window procedure.
    */
    g_hInstance = hInstance;
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    //Initialize the Window class
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);                    //class struct size in bytes
    wc.style = CS_HREDRAW | CS_VREDRAW;                //class style (CS_)
    wc.lpfnWndProc = WindowProc;                       //long pointer to window procedure function
    wc.cbClsExtra = 0;                                 //extra bytes to allocate for the class
    wc.cbWndExtra = 0;                                 //extra bytes to allocate for the window instance
    wc.hInstance = hInstance;                          //instance handle
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);        //icon (IDI_)
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);          //curosr (IDC_)
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW +1);      //background brush handle
    wc.lpszMenuName = NULL;                            //resource name of class menu
    wc.lpszClassName = "Editor Class";                 //class name string
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);      //small version of the icon (IDI_)
    
    /*
    *   The Window class only needs to be registered once.
    *   If there is a previous instance handle we do not register the class again.
    */
    if (!hPrevious)
    {
        //Register the window class we just initialized (wc)
        if(!RegisterClassEx(&wc))
        {
            //This should never happen, but we inform the user and return gracefully in case it does.
            MessageBox(NULL, "Window Registration Failed", "Error", MB_ICONERROR | MB_OK);
            return 0;
        }
    }

    //Initialize the menu bar
    HMENU hMenu = CreateMenu();

    //Initialize the File submenu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_NEW, "&New");
    AppendMenu(hFileMenu, MF_STRING, IDM_OPEN, "&Open...");
    AppendMenu(hFileMenu, MF_STRING, IDM_SAVE, "&Save...");
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, "E&xit");

    //Append the File submenu and about button to the menu bar
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "&File");
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, "&About");

    //Initialize the main window
    g_hWnd = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,                              //Extended window style (WS_EX_)
        "Editor Class",                                //Class name
        "dtree",                                     //Title bar text
        WS_OVERLAPPEDWINDOW,                           //Window style (WS_)
        CW_USEDEFAULT, CW_USEDEFAULT,                  //Default X and Y screen position of window
        WINDOW_WIDTH, WINDOW_HEIGHT,                   //Width and Height of the window
        NULL,                                          //Parent window (there isn't one, this is the main window)
        hMenu,                                         //Menu handle
        hInstance,                                     //Instance Handle
        NULL                                           //lParam (not used here)
    );

    if(g_hWnd == NULL)
    {
        //Again, this should never happen since we just created g_Hwnd, but just in case, inform the user and return gracefully.
        MessageBox(NULL, "Window creation failed.", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    /*
    *   Now that the main window handle is set up, we can start initializing the individual ui-element "windows" in our main window
    *   For organization sake, we do this in the InitializeUI function.
    */
    InitializeUI(g_hWnd);

    //Finally, now that the window is fully initialized, we can show it and begin ticking the message loop.
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

/*=============================================================================
*   WindowProc [LRESULT]
*       The win32 window procedure
=============================================================================*/
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        //When the Window is created:
        case WM_CREATE:
        {
            //Do nothing... for now...
            break;
        }
        
        //When the Window is resized:
        case WM_SIZE:
        {
            //lParam holds the new window size x and y as 16-bit WORDs
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            //If the window height is made shorter or longer, readjust the treeview height.
            SetWindowPos(g_hTreeView, NULL, 0, 0, TREEVIEW_WIDTH, height, SWP_NOZORDER);

            //If the window width is made narrower or wider, readjust the editor width.
            int editLeft = TREEVIEW_WIDTH + 10;
            int editWidth = width - TREEVIEW_WIDTH - 20;
            SetWindowPos(g_hEditName, NULL, editLeft, 10, editWidth, 25, SWP_NOZORDER);
            SetWindowPos(g_hEditDesc, NULL, editLeft, 70, editWidth, 100, SWP_NOZORDER);
            SetWindowPos(g_hSaveButton, NULL, editLeft, 180, 100, 30, SWP_NOZORDER);
        }
        break;

        //When a Command is sent to the window:
        case WM_COMMAND:
        {
            //wParam for a menu holds 
            switch(LOWORD(wParam))
            {

                case IDM_NEW:
                {
                    TreeView_DeleteAllItems(g_hTreeView);
                    SetWindowText(g_hEditName, "");
                    SetWindowText(g_hEditDesc, "");
                    strcpy(g_szFileName, "");
                    break;
                }

                case IDM_OPEN:
                {
                    char szFile[MAX_PATH] = {0};

                    //Initialize an OPENFILENAME struct to be used by GetOpenFileName
                    OPENFILENAME ofn = {0};
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    //Display an open file dialog
                    if (GetOpenFileName(&ofn))
                    {
                        strcpy(g_szFileName, szFile);
                        //Clear the treeview
                        TreeView_DeleteAllItems(g_hTreeView);
                        //Load the file data into the treeview from the given path
                        LoadTreeFromFile(g_hTreeView, szFile);
                    }
                    break;
                }

                case IDM_SAVE:
                {
                    char szFile[MAX_PATH] = {0};

                    //If this is a new file with no name globally defined
                    if(g_szFileName[0] == '\0')
                    {
                        //Initialize an OPENFILENAME struct to be used by GetSaveFileName
                        OPENFILENAME ofn = {0};
                        strcpy(szFile, "untitled.dat");
                        ofn.lStructSize = sizeof(OPENFILENAME);
                        ofn.hwndOwner = hWnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_OVERWRITEPROMPT;

                        //Dispaly a save file dialog
                        if(GetSaveFileName(&ofn))
                        {
                            strcpy(g_szFileName, szFile);
                            //Save our data
                            SaveTreeToFile(g_hTreeView, szFile);
                        }
                    }
                    else
                    {
                        //Simply save the data if we're editing an open file
                        SaveTreeToFile(g_hTreeView, g_szFileName);
                    }
                }
                break;

                case IDM_EXIT:
                {
                    DestroyWindow(hWnd);
                    break;
                }

                case IDM_ABOUT:
                {
                    //Show an about dialog
                    MessageBox(hWnd, "dtree", "About", MB_OK | MB_ICONINFORMATION);
                    break;
                }

                //Save individual tree items
                case ID_SAVE_ITEM:
                {
                    if(g_hSelectedItem != NULL)
                    {
                        SaveItemChanges(g_hTreeView, g_hSelectedItem);
                    }
                    break;
                }
            }
            break;
        }

        //Called on special window events
        case WM_NOTIFY:
        {
            NMHDR* pnmhdr = (NMHDR*)lParam;
            if(pnmhdr->idFrom == ID_TREEVIEW)
            {
                switch(pnmhdr->code)
                {
                    //When the selection of our treeview is changed:
                    case TVN_SELCHANGED:
                    {
                        NMTREEVIEW* pnmtv = (NMTREEVIEW*)lParam;
                        g_hSelectedItem = pnmtv->itemNew.hItem;
                        UpdateEditFields(g_hTreeView, g_hSelectedItem);
                    }
                    break;

                    //When the mouse right clicks our tree view
                    case NM_RCLICK:
                    {
                        POINT pt;
                        GetCursorPos(&pt);

                        HMENU hPopupMenu = CreatePopupMenu();
                        AppendMenu(hPopupMenu, MF_STRING, 1001, "Add Child Item");
                        AppendMenu(hPopupMenu, MF_STRING, 1002, "Delete Item");

                        //Create a popup menu at the mouse position
                        int cmd = TrackPopupMenu(hPopupMenu, TPM_RETURNCMD | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                        DestroyMenu(hPopupMenu);

                        switch(cmd)
                        {
                            //Create an item:
                            case 1001:
                            {
                                CreateNewItem(g_hTreeView, g_hSelectedItem);
                                break;
                            }
                            //Delete an item:
                            case 1002:
                            {
                                //Only if something is selected
                                if(g_hSelectedItem != NULL)
                                {
                                    //Get pointer to the selected item
                                    TreeNodeData* data = (TreeNodeData*)TreeView_GetItem(g_hTreeView, g_hSelectedItem);

                                    if(data)
                                    {
                                        //Free the memory occupied by the selected item
                                        free(data);
                                    }
                                    //Remove it from the tree view
                                    TreeView_DeleteItem(g_hTreeView, g_hSelectedItem);
                                    //Deselect it (it doesn't exist anymore!)
                                    g_hSelectedItem = NULL;
                                    //Clear the contents of the editor
                                    SetWindowText(g_hEditName, "");
                                    SetWindowText(g_hEditDesc, "");
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
        break;
        //Called on DestroyWindow(hWnd)
        case WM_DESTROY:
        {
            //Request that the system terminate the application thread
            PostQuitMessage(0);
            break;
        }
        default:
        {
            //return the default window procedure
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }
    return 0;
}

/*=============================================================================
*   InitializeUI [void]
*       Construct/Create the "windows" for each of our individual UI elements
*
*       Parameters:
*           HWND hWnd - The main window handle that will be used as the parent for the ui elements
*
=============================================================================*/
void InitializeUI(HWND hWnd)
{
    //Create the Tree View box
    g_hTreeView = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0,
        0,
        TREEVIEW_WIDTH,
        0,
        hWnd,
        (HMENU)ID_TREEVIEW,
        g_hInstance,
        NULL
    );

    //Create the Name TextBlock
    HWND hNameLabel = CreateWindow
    (
        "STATIC", 
        "Name:",
        WS_VISIBLE | WS_CHILD,
        TREEVIEW_WIDTH + 10, 10,
        100, 20,
        hWnd,
        NULL,
        g_hInstance,
        NULL
    );

    //Create the Description TextBlock
    HWND hDescLabel = CreateWindow
    (
        "STATIC", 
        "Description:",
        WS_VISIBLE | WS_CHILD,
        TREEVIEW_WIDTH + 10, 50,
        100, 20,
        hWnd,
        NULL,
        g_hInstance,
        NULL
    );

    //Create the Name editor TextBox
    g_hEditName = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        TREEVIEW_WIDTH + 10, 30,
        300, 25,
        hWnd,
        (HMENU)ID_EDIT_NAME, 
        g_hInstance,
        NULL
    );

    //Create the Description editor TextBox
    g_hEditDesc = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        TREEVIEW_WIDTH + 10, 70,
        300, 100,
        hWnd,
        (HMENU)ID_EDIT_DESCRIPTION,
        g_hInstance,
        NULL
    );

    //Create a save button
    g_hSaveButton = CreateWindow
    (
        "BUTTON",
        "Save Item",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        TREEVIEW_WIDTH + 10, 180,
        100, 30,
        hWnd,
        (HMENU)ID_SAVE_ITEM, 
        g_hInstance,
        NULL
    );

    //Construct a new root node and copy it's data to the Tree View
    TreeNodeData* rootData = (TreeNodeData*)malloc(sizeof(TreeNodeData));
    strcpy(rootData->name, "Root");
    strcpy(rootData->description, "This is the root node");
    AddItemToTree(g_hTreeView, NULL, "Root", rootData);
}

/*=============================================================================
*   AddItemToTree [void]
*       Add an item to a TreeView using a TreeView insert struct
*
*       Parameters:
*           HWND hTreeView - Handle to the treeview window control
*           HTREEITEM hParent - Handle to the new item's parent in the tree hierarchy
*           char* text - Pointer to label of the new item
*           TreeNodeData* - Pointer to TreeNodeData to be associated with the new item
*
=============================================================================*/
void AddItemToTree(HWND hTreeView, HTREEITEM hParent, const char* text, TreeNodeData* data)
{
    TVINSERTSTRUCT tvins;
    //Clear enough memory for the TreeView insert struct
    ZeroMemory(&tvins, sizeof(tvins));

    tvins.hParent = hParent;
    tvins.hInsertAfter = TVI_LAST;
    tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvins.item.pszText = (LPSTR)text;
    tvins.item.lParam = (LPARAM)data;

    HTREEITEM hItem = TreeView_InsertItem(hTreeView, &tvins);

    //If there is no parent defined, then this is a root item.
    if(hParent == NULL)
    {
        //Expand the root item automatically...
        TreeView_Expand(hTreeView, hItem, TVE_EXPAND);
    }
}

/*=============================================================================
*   CreateNewItem [void]
*       Create and add a completely new item to the TreeView
*
*       Parameters:
*           HWND hTreeView - Handle to the treeview window control
*           HTREEITEM hParent - Handle to the new item's parent in the tree hierarchy
*
=============================================================================*/
void CreateNewItem(HWND hTreeView, HTREEITEM hParent)
{
    //Allocate memory for the new TreeNodeData
    TreeNodeData* data = (TreeNodeData*)malloc(sizeof(TreeNodeData));

    //Set the default values
    strcpy(data->name, "New Item");
    strcpy(data->description, "");

    //Add the new item to the TreeView
    AddItemToTree(hTreeView, hParent, "New Item", data);
}

/*=============================================================================
*   UpdateEditFields [void]
*       Used to copy the contents TreeItem to the editor controls
*
*       Parameters:
*           HWND hTreeView - Handle to the TreeView window control
*           HTREEITEM hItem - Handle to the TreeItem we are copying content from 
*
=============================================================================*/
void UpdateEditFields(HWND hTreeView, HTREEITEM hItem)
{
    if (hItem == NULL)
    {
        return;
        
    }
    else
    {
        //Get the item data from the given hItem and set the Window text accordingly
        TreeNodeData* data = (TreeNodeData*)TreeView_GetItem(hTreeView, hItem);
        if(data)
        {
            SetWindowText(g_hEditName, data->name);
            SetWindowText(g_hEditDesc, data->description);
        }
    }
}

/*=============================================================================
*   SaveItemChanges [void]
*       Saves data from our editors into a TreeItem
*
*       Parameters:
*           HWND hTreeView - Handle to the TreeView window control
*           HTREEITEM hItem - Handle to the TreeItem we are saving data into
*
=============================================================================*/
void SaveItemChanges(HWND hTreeView, HTREEITEM hItem)
{
    if(hItem == NULL)
    {
        //Don't do anything if it's null...
        return;
    }
    else
    {
        TreeNodeData* data = (TreeNodeData*)TreeView_GetItem(hTreeView, hItem);
        if(data)
        {
            char buffer[MAX_LOADSTRING];

            GetWindowText(g_hEditDesc, buffer, MAX_LOADSTRING);
            strcpy(data->description, buffer);

            TVITEM item;
            ZeroMemory(&item, sizeof(item));
            item.mask = TVIF_TEXT | TVIF_HANDLE;
            item.hItem = hItem;
            item.pszText = data->name;
            TreeView_SetItem(hTreeView, &item);
        }
    }
}

/*=============================================================================
*   RecursiveSaveTree [HTREEITEM]
*
*       Parameters:
*           HWND hTreeView - The TreeView that is to be saved
*           HTREEITEM hItem - The root item we save from
*           FILE* file - Pointer to file stream
*           int level - How far into the hierarchy we are when this is called
*
=============================================================================*/
HTREEITEM RecursiveSaveTree(HWND hTreeView, HTREEITEM hItem, FILE* file, int level)
{

    char escapedDesc[MAX_LOADSTRING * 2];
    int j=0;

    //End the recusion when we have iterated through the entire tree
    if (hItem == NULL)
    {
        return NULL;
    }

    else
    {
        TreeNodeData* data = (TreeNodeData*)TreeView_GetItem(hTreeView, hItem);
        if(data)
        {
            //Write the Tree to the file using fprintf and the provided ptr to file handle
            for(int i = 0; i < level; i++)
            {
                fprintf(file, "\t");
            }
            fprintf(file, "%s\n", data->name);

            for(int i=0; i< level; i++)
            {
                fprintf(file, "\t");
            }
            fprintf(file, "{\n");

            for(int i=0; i < level + 1; i++)
            {
                fprintf(file, "\t");
            }

            for(int i = 0; i< strlen(data->description); i++)
            {
                if(data->description[i] == '\n')
                {
                    escapedDesc[j++] = '\\';
                    escapedDesc[j++] = 'n';
                }
                else
                {
                    escapedDesc[j++] = data->description[i];
                }
                escapedDesc[j] = '\0';
            }

            fprintf(file, "%s\n", escapedDesc);

            //Iterate through the children of each item
            HTREEITEM hChild = TreeView_GetChild(hTreeView, hItem);
            while(hChild != NULL)
            {
                hChild = RecursiveSaveTree(hTreeView, hChild, file, level+1);
            }
            for(int i=0; i< level; i++)
            {
                fprintf(file, "\t");
            }
            fprintf(file, "}\n");
        }

        //Will either return another root node or return NULL when we are finished saving
        return TreeView_GetNextSibling(hTreeView, hItem);
    }
}

/*=============================================================================
*   SaveTreeToFile [void]
*
*       Parameters:
*           HWND hTreeView - The Tree we want to pass on to RecursiveSaveTree
*           char* fileName - FileName we received either from opening a file or from a file save dialog
*
=============================================================================*/
void SaveTreeToFile(HWND hTreeView, const char* fileName)
{
    FILE* file = fopen(fileName, "w");
    if(file)
    {
        HTREEITEM hRoot = TreeView_GetRoot(hTreeView);
        while(hRoot != NULL)
        {
            hRoot = RecursiveSaveTree(hTreeView, hRoot, file, 0);
        }
        fclose(file);
        MessageBox(g_hWnd, "Tree saved successfully", "Save", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBox(g_hWnd, "Failed to save tree", "Error", MB_OK | MB_ICONERROR);
    }
}

/*=============================================================================
*   ParseLine [void]
*
*       Parameters:
*           char* line - Input line with escape sequence
*           char* output - Processed line with correct newline chars
*
=============================================================================*/
void ParseLine(char* line, char* output)
{
    int j = 0;
    //Iterate each character
    for(int i=0; i<strlen(line); i++)
    {
        //Find escape sequence
        if(line[i] == '\\' && line[i+1] == 'n')
        {
            //Output a newline character
            output[j++] = '\n';
            i++;
        }
        else
        {
            //copy directly to output if no escape sequence found
            output[j++] = line[i];
        }
        //Terminate the output string with \0
        output[j]='\0';
    }
}

/*=============================================================================
*   LoadTreeFromFile [void]
*
*       Parameters:
*           HWND hTreeView - The TreeView we are going to load data into
*           char* fileName - The open file name we received from the dialog
*
=============================================================================*/
void LoadTreeFromFile(HWND hTreeView, const char* fileName)
{
    FILE* file = fopen(fileName, "r");
    if(file)
    {
        TreeView_DeleteAllItems(hTreeView);
        char line[MAX_LOADSTRING * 2];
        if(fgets(line, sizeof(line), file))
        {
            line[strcspn(line, "\r\n")] = 0;

            TreeNodeData* rootData = (TreeNodeData*)malloc(sizeof(TreeNodeData));
            strcpy(rootData->name, line);
            
            fgets(line, sizeof(line), file);
            fgets(line, sizeof(line), file);
            line[strcspn(line, "\r\n")] = 0;

            ParseLine(line, rootData->description);

            TVINSERTSTRUCT tvis;
            tvis.hParent = NULL;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvis.item.pszText = rootData->name;
            tvis.item.lParam = (LPARAM)rootData;
            HTREEITEM hRoot = TreeView_InsertItem(hTreeView, &tvis);

            TreeView_Expand(hTreeView, hRoot, TVE_EXPAND);
            //Load children
            RecursiveLoadTree(hTreeView, hRoot, file, 1);
        }
        fclose(file);
    }
    else
    {
        //Inform the user if for some reason loading fails.
        MessageBox(g_hWnd, "Failed to open file", "Error", MB_OK | MB_ICONERROR);
    }
}

/*=============================================================================
*   RecursiveLoadTree [HTREEITEM]
*
*       Parameters:
*           HWND hTreeView - The TreeView we are going to load data into
*           HTREEITEM hItem - The current item being loaded
*           FILE* file - Pointer to file stream
*           int level - How far into the hierarchy we are when this is called
*
=============================================================================*/
HTREEITEM RecursiveLoadTree(HWND hTreeView, HTREEITEM hParent, FILE* file, int level)
{
    char line[MAX_LOADSTRING * 2];
    char indent[MAX_LOADSTRING];

    //Create tab indents for whatever the current level is
    for(int i=0; i< level; i++)
    {
        indent[i] = '\t';
    }
    indent[level] = '\0';

    while(fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\r\n")] = 0;

        //check if this is the closing brace for our level
        char closingBrace[MAX_LOADSTRING];
        sprintf(closingBrace, "%s}", indent);
        closingBrace[level] = '}';
        closingBrace[level + 1] = '\0';
        if(strcmp(line, closingBrace) == 0)
        {
            //End of the current level
            return NULL;
        }

        //check if this is the node at our level
        if(strncmp(line, indent, level) == 0 && line[level] != '\t' && line[level] != '{')
        {
            //Child Node
            char* nodeName = &line[level];

            //Call twice since there are is a blank line
            fgets(line, sizeof(line), file);

            //Load descriptions
            fgets(line, sizeof(line), file);
            line[strcspn(line, "\r\n")] = 0;

            //Create a new TreeNodeData and copy the loaded line to it
            TreeNodeData* nodeData = (TreeNodeData*)malloc(sizeof(TreeNodeData));
            strcpy(nodeData->name, nodeName);
            ParseLine(&line[level + 1], nodeData->description);

            //Add to TreeView
            TVINSERTSTRUCT tvis;
            tvis.hParent = hParent;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvis.item.pszText = nodeData->name;
            tvis.item.lParam = (LPARAM)nodeData;
            HTREEITEM hItem = TreeView_InsertItem(hTreeView, &tvis);

            //Continue loading children
            RecursiveLoadTree(hTreeView, hItem, file, level + 1);
        }
    }
    
    return NULL;
}