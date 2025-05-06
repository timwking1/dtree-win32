/*=============================================================================
*       dtree.c
*       timwking1
*       20-Mar 2025
=============================================================================*/
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")

/*=============================================================================
*   Constants
=============================================================================*/

#define MAX_LOADSTRING 255

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define TREEVIEW_WIDTH 250
#define TREEVIEW_HEIGHT 600

#define IDM_NEW 101
#define IDM_OPEN 102
#define IDM_SAVE 103
#define IDM_EXIT 104
#define IDM_ABOUT 105

#define ID_TREEVIEW 201
#define ID_EDIT_NAME 202
#define ID_EDIT_DESCRIPTION 203

/*=============================================================================
*   Global Declarations
=============================================================================*/

HINSTANCE hMainInstance;
HWND hMainWindow;
HWND hTreeView;
HWND hNameEditWindow;
HWND hDescEditWindow;

HTREEITEM hSelectedItem;
wchar_t g_szFileName[MAX_PATH] = L"";

/*=============================================================================
*   Struct Definitions
=============================================================================*/

typedef struct _TreeNodeData 
{
    wchar_t name[MAX_LOADSTRING];
    wchar_t description[MAX_LOADSTRING];
} TreeNodeData;

/*=============================================================================
*   Declarations
=============================================================================*/

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void InitializeUI(HWND hwnd);
void AddItemToTree(HWND hTreeView, HTREEITEM hParent, TreeNodeData* data);
void SaveTreeToFile(HWND hTreeView, const wchar_t* fileName);
void LoadTreeFromFile(HWND hTreeView, const wchar_t* fileName);
void UpdateEditFields(HWND hTreeView, HTREEITEM hItem);
void OnItemChanges(HWND hTreeView, HTREEITEM hItem);
HTREEITEM RecursiveSaveTree(HWND hTreeView, HTREEITEM hItem, FILE* file, int level);
HTREEITEM RecursiveLoadTree(HWND hTreeView, HTREEITEM hParent, FILE* file, int level);
void CreateNewItem(HWND hTreeView, HTREEITEM hParent);

/*=============================================================================
*   WinMain 
*       The entry point of a win32 application
*       ***gcc REQUIRES the -mwindows flag to recognize this as the entry point!***
=============================================================================*/
int FAR PASCAL WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevious, WCHAR *lpCmdLine, int nCmdShow)
{
    /*
    *   Initialize the instance handle reference and the common controls
    *   used in the window procedure.
    */
    hMainInstance = hInstance;
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
    wc.lpszClassName = L"Editor Class";                //class name string
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
            MessageBox(NULL, L"Window Registration Failed", L"Error", MB_ICONERROR | MB_OK);
            return 0;
        }
    }

    //Initialize the menu bar
    HMENU hMenu = CreateMenu();

    //Initialize the File submenu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_NEW, L"&New");
    AppendMenu(hFileMenu, MF_STRING, IDM_OPEN, L"&Open...");
    AppendMenu(hFileMenu, MF_STRING, IDM_SAVE, L"&Save...");
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, L"E&xit");

    //Append the File submenu and about button to the menu bar
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, L"&About");

    //Initialize the main window
    hMainWindow = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,                              //Extended window style (WS_EX_)
        L"Editor Class",                                //Class name
        L"dtree",                                     //Title bar text
        WS_OVERLAPPEDWINDOW,                           //Window style (WS_)
        CW_USEDEFAULT, CW_USEDEFAULT,                  //Default X and Y screen position of window
        WINDOW_WIDTH, WINDOW_HEIGHT,                   //Width and Height of the window
        NULL,                                          //Parent window (there isn't one, this is the main window)
        hMenu,                                         //Menu handle
        hInstance,                                     //Instance Handle
        NULL                                           //lParam (not used here)
    );

    if(hMainWindow == NULL)
    {
        //Again, this should never happen since we just created hMainWindow, but just in case, inform the user and return gracefully.
        MessageBox(NULL, L"Window creation failed.", L"Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    /*
    *   Now that the main window handle is set up, we can start initializing the individual ui-element "windows" in our main window
    *   For organization sake, we do this in the InitializeUI function.
    */
    InitializeUI(hMainWindow);

    //Finally, now that the window is fully initialized, we can show it and begin ticking the message loop.
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
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
            SetWindowPos(hTreeView, NULL, 0, 0, TREEVIEW_WIDTH, height, SWP_NOZORDER);

            //If the window width is made narrower or wider, readjust the editor width.
            int editLeft = TREEVIEW_WIDTH + 10;
            int editWidth = width - TREEVIEW_WIDTH - 20;
            SetWindowPos(hNameEditWindow, NULL, editLeft, 10, editWidth, 25, SWP_NOZORDER);
            SetWindowPos(hDescEditWindow, NULL, editLeft, 70, editWidth, 100, SWP_NOZORDER);
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
                    TreeView_DeleteAllItems(hTreeView);
                    SetWindowText(hNameEditWindow, L"");
                    SetWindowText(hDescEditWindow, L"");
                    wcscpy(g_szFileName, L"");
                    break;
                }

                case IDM_OPEN:
                {
                    wchar_t szFile[MAX_PATH] = {0};

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
                        wcscpy(g_szFileName, szFile);
                        //Clear the treeview
                        TreeView_DeleteAllItems(hTreeView);
                        //Load the file data into the treeview from the given path
                        LoadTreeFromFile(hTreeView, szFile);
                    }
                    break;
                }

                case IDM_SAVE:
                {
                    wchar_t szFile[MAX_PATH] = {0};

                    //If this is a new file with no name globally defined
                    if(g_szFileName[0] == '\0')
                    {
                        //Initialize an OPENFILENAME struct to be used by GetSaveFileName
                        OPENFILENAME ofn = {0};
                        wcscpy(szFile, L"untitled.dat");
                        ofn.lStructSize = sizeof(OPENFILENAME);
                        ofn.hwndOwner = hWnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_OVERWRITEPROMPT;

                        //Dispaly a save file dialog
                        if(GetSaveFileName(&ofn))
                        {
                            wcscpy(g_szFileName, szFile);
                            //Save our data
                            SaveTreeToFile(hTreeView, szFile);
                        }
                    }
                    else
                    {
                        //Simply save the data if we're editing an open file
                        SaveTreeToFile(hTreeView, g_szFileName);
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
                    MessageBox(hWnd, L"dtree", L"About", MB_OK | MB_ICONINFORMATION);
                    break;
                }

                // Handle edit control changes
                case ID_EDIT_NAME:
                {
                    if(HIWORD(wParam) == EN_CHANGE && hSelectedItem != NULL)
                    {
                        OnItemChanges(hTreeView, hSelectedItem);
                    }
                    break;
                }
                case ID_EDIT_DESCRIPTION:
                {
                    if (HIWORD(wParam) == EN_CHANGE && hSelectedItem != NULL)
                    {
                        OnItemChanges(hTreeView, hSelectedItem);
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
                        hSelectedItem = pnmtv->itemNew.hItem;
                        UpdateEditFields(hTreeView, hSelectedItem);
                    }
                    break;

                    //When the mouse right clicks our tree view
                    case NM_RCLICK:
                    {
                        POINT pt;
                        GetCursorPos(&pt);

                        HMENU hPopupMenu = CreatePopupMenu();
                        AppendMenu(hPopupMenu, MF_STRING, 1001, L"Add Child Item");
                        AppendMenu(hPopupMenu, MF_STRING, 1002, L"Delete Item");

                        //Create a popup menu at the mouse position
                        int cmd = TrackPopupMenu(hPopupMenu, TPM_RETURNCMD | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                        DestroyMenu(hPopupMenu);

                        switch(cmd)
                        {
                            //Create an item:
                            case 1001:
                            {
                                CreateNewItem(hTreeView, hSelectedItem);
                                break;
                            }
                            //Delete an item:
                            case 1002:
                            {
                                //Only if something is selected
                                if(hSelectedItem != NULL)
                                {
                                    //Get pointer to the selected item
                                    TreeNodeData* data = (TreeNodeData*)TreeView_GetItem(hTreeView, hSelectedItem);

                                    if(data)
                                    {
                                        //Free the memory occupied by the selected item
                                        free(data);
                                    }
                                    //Remove it from the tree view
                                    TreeView_DeleteItem(hTreeView, hSelectedItem);
                                    //Deselect it (it doesn't exist anymore!)
                                    hSelectedItem = NULL;
                                    //Clear the contents of the editor
                                    SetWindowText(hNameEditWindow, L"");
                                    SetWindowText(hDescEditWindow, L"");
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
    hTreeView = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0,
        0,
        TREEVIEW_WIDTH,
        0,
        hWnd,
        (HMENU)ID_TREEVIEW,
        hMainInstance,
        NULL
    );

    //Create the Name TextBlock
    HWND hNameLabel = CreateWindow
    (
        L"STATIC", 
        L"Name:",
        WS_VISIBLE | WS_CHILD,
        TREEVIEW_WIDTH + 10, 10,
        100, 20,
        hWnd,
        NULL,
        hMainInstance,
        NULL
    );

    //Create the Description TextBlock
    HWND hDescLabel = CreateWindow
    (
        L"STATIC", 
        L"Description:",
        WS_VISIBLE | WS_CHILD,
        TREEVIEW_WIDTH + 10, 50,
        100, 20,
        hWnd,
        NULL,
        hMainInstance,
        NULL
    );

    //Create the Name editor TextBox
    hNameEditWindow = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        TREEVIEW_WIDTH + 10, 30,
        300, 25,
        hWnd,
        (HMENU)ID_EDIT_NAME, 
        hMainInstance,
        NULL
    );

    //Create the Description editor TextBox
    hDescEditWindow = CreateWindowEx
    (
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        TREEVIEW_WIDTH + 10, 70,
        300, 100,
        hWnd,
        (HMENU)ID_EDIT_DESCRIPTION,
        hMainInstance,
        NULL
    );

    //Construct a new root node and copy it's data to the Tree View
    TreeNodeData* rootData = (TreeNodeData*)malloc(sizeof(TreeNodeData));
    wcscpy(rootData->name, L"Root");
    wcscpy(rootData->description, L"This is the root node");
    AddItemToTree(hTreeView, NULL, rootData);
}

/*=============================================================================
*   AddItemToTree [void]
*       Add an item to a TreeView using a TreeView insert struct
*
*       Parameters:
*           HWND hTreeView - Handle to the treeview window control
*           HTREEITEM hParent - Handle to the new item's parent in the tree hierarchy
*           TreeNodeData* - Pointer to TreeNodeData to be associated with the new item
*
=============================================================================*/
void AddItemToTree(HWND hTreeView, HTREEITEM hParent, TreeNodeData* data)
{
    TVINSERTSTRUCT tvins;
    ZeroMemory(&tvins, sizeof(tvins));
    tvins.hParent = hParent;
    tvins.hInsertAfter = TVI_LAST;
    tvins.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvins.item.pszText = (LPWSTR)data->name;
    tvins.item.lParam = (LPARAM)data;
    HTREEITEM hItem = TreeView_InsertItem(hTreeView, &tvins);
    
    if(hParent != NULL)
    {
        //TreeView_Expand(hTreeView, hItem, TVE_EXPAND);
        TreeView_Expand(hTreeView, hParent, TVE_EXPAND);
    }

    UpdateEditFields(hTreeView, hItem);
    // Force the TreeView to update its display
    UpdateWindow(hTreeView);
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
    wcscpy(data->name, L"New Item");
    wcscpy(data->description, L"");

    //Add the new item to the TreeView
    AddItemToTree(hTreeView, hParent, data);
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
        SetWindowText(hNameEditWindow, L"");
        SetWindowText(hDescEditWindow, L"");
        return;
    }
    TVITEMA item = {0};
    item.mask  = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(hTreeView, &item)) return;

    TreeNodeData* data = (TreeNodeData*)item.lParam;
    if (!data) return;

    SetWindowText(hNameEditWindow, data->name);
    SetWindowText(hDescEditWindow, data->description);
}

/*=============================================================================
*   OnItemChanges [void]
*       Saves data from our editors into a TreeItem
*
*       Parameters:
*           HWND hTreeView - Handle to the TreeView window control
*           HTREEITEM hItem - Handle to the TreeItem we are saving data into
*
=============================================================================*/
void OnItemChanges(HWND hTreeView, HTREEITEM hItem)
{
    if(!hItem)
    {
        return;
    }

    TVITEMW item = {};
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    
    if(!TreeView_GetItem(hTreeView, &item))
    {
        return;
    }

    TreeNodeData* data = (TreeNodeData*)(item.lParam);
    if(data == NULL)
    {
        return;
    }

    GetWindowText(hNameEditWindow, data->name, MAX_LOADSTRING);
    GetWindowText(hDescEditWindow, data->description, MAX_LOADSTRING);

    item.mask = TVIF_TEXT;
    item.pszText = data->name;
    TreeView_SetItem(hTreeView, &item);
    UpdateWindow(hTreeView);
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
    wchar_t escapedDesc[MAX_LOADSTRING * 2];
    int j=0;

    //End the recursion when we have iterated through the entire tree
    if (hItem == NULL)
    {
        return NULL;
    }

    TVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    
    if (TreeView_GetItem(hTreeView, &item))
    {
        TreeNodeData* data = (TreeNodeData*)item.lParam;
        if(data)
        {
            //Write the Tree to the file using fprintf and the provided ptr to file handle
            for(int i = 0; i < level; i++)
            {
                fwprintf(file, L"\t");
            }
            fwprintf(file, L"%s\n", data->name);

            for(int i=0; i< level; i++)
            {
                fwprintf(file, L"\t");
            }
            fwprintf(file, L"{\n");

            for(int i=0; i < level + 1; i++)
            {
                fwprintf(file, L"\t");
            }

            for(int i = 0; i< wcslen(data->description); i++)
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

            fwprintf(file, L"%s\n", escapedDesc);

            //Iterate through the children of each item
            HTREEITEM hChild = TreeView_GetChild(hTreeView, hItem);
            while(hChild != NULL)
            {
                hChild = RecursiveSaveTree(hTreeView, hChild, file, level+1);
            }
            for(int i=0; i< level; i++)
            {
                fwprintf(file, L"\t");
            }
            fwprintf(file, L"}\n");
        }
    }

    //Will either return another root node or return NULL when we are finished saving
    return TreeView_GetNextSibling(hTreeView, hItem);
}

/*=============================================================================
*   SaveTreeToFile [void]
*
*       Parameters:
*           HWND hTreeView - The Tree we want to pass on to RecursiveSaveTree
*           char* fileName - FileName we received either from opening a file or from a file save dialog
*
=============================================================================*/
void SaveTreeToFile(HWND hTreeView, const wchar_t* fileName)
{
    FILE* file = _wfopen(fileName, L"w");
    if(file)
    {
        HTREEITEM hRoot = TreeView_GetRoot(hTreeView);
        while(hRoot != NULL)
        {
            hRoot = RecursiveSaveTree(hTreeView, hRoot, file, 0);
        }
        fclose(file);
        MessageBox(hMainWindow, L"Tree saved successfully", L"Save", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBox(hMainWindow, L"Failed to save tree", L"Error", MB_OK | MB_ICONERROR);
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
void ParseLine(wchar_t* line, wchar_t* output)
{
    int j = 0;
    //Iterate each character
    for(int i=0; i<wcslen(line); i++)
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
void LoadTreeFromFile(HWND hTreeView, const wchar_t* fileName)
{
    FILE* file = _wfopen(fileName, L"r");
    if(file)
    {
        TreeView_DeleteAllItems(hTreeView);
        wchar_t line[MAX_LOADSTRING * 2];
        if(fgetws(line, sizeof(line), file))
        {
            line[wcscspn(line, L"\r\n")] = 0;

            TreeNodeData* rootData = (TreeNodeData*)malloc(sizeof(TreeNodeData));
            wcscpy(rootData->name, line);
            
            fgetws(line, sizeof(line), file);
            fgetws(line, sizeof(line), file);
            line[wcscspn(line, L"\r\n")] = 0;

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
        MessageBoxW(hMainWindow, L"Failed to open file", L"Error", MB_OK | MB_ICONERROR);
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
    wchar_t line[MAX_LOADSTRING * 2];
    wchar_t indent[MAX_LOADSTRING];

    //Create tab indents for whatever the current level is
    for(int i=0; i< level; i++)
    {
        indent[i] = '\t';
    }
    indent[level] = '\0';

    while(fgetws(line, sizeof(line), file))
    {
        line[wcscspn(line, L"\r\n")] = 0;

        //check if this is the closing brace for our level
        wchar_t closingBrace[MAX_LOADSTRING];
        swprintf(closingBrace, MAX_LOADSTRING, L"%s}", indent);
        closingBrace[level] = '}';
        closingBrace[level + 1] = '\0';
        if(wcscmp(line, closingBrace) == 0)
        {
            //End of the current level
            return NULL;
        }

        //check if this is the node at our level
        if(wcsncmp(line, indent, level) == 0 && line[level] != '\t' && line[level] != '{')
        {
            //Child Node
            wchar_t* nodeName = &line[level];

            //Skip the opening brace line
            fgetws(line, sizeof(line), file);

            //Load description
            fgetws(line, sizeof(line), file);
            line[wcscspn(line, L"\r\n")] = 0;

            //Create a new TreeNodeData and copy the loaded line to it
            TreeNodeData* nodeData = (TreeNodeData*)malloc(sizeof(TreeNodeData));
            wcscpy(nodeData->name, nodeName);
            
            //Parse the description, skipping the indentation
            ParseLine(&line[level + 1], nodeData->description);

            //Add to TreeView
            TVINSERTSTRUCTW tvis;
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