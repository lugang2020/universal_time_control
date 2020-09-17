//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#include <windows.h>      // For common windows data types and function headers
#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>
#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
#include <propvarutil.h>  // for PROPVAR-related functions
#include <propkey.h>      // for the Property key APIs/datatypes
#include <propidl.h>      // for the Property System APIs
#include <strsafe.h>      // for StringCchPrintfW
#include <shtypes.h>      // for COMDLG_FILTERSPEC
#include <new>

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
	{L"exe (*.exe)",       L"*.exe"},
	/*{L"Web Page (*.htm; *.html)",    L"*.htm;*.html"},
	{L"Text Document (*.txt)",       L"*.txt"},
	{L"All Documents (*.*)",         L"*.*"}*/
};

// Indices of file types
#define INDEX_EXE 1
#define INDEX_WEBPAGE 2
#define INDEX_TEXTDOC 3

// Controls
#define CONTROL_GROUP           2000
#define CONTROL_RADIOBUTTONLIST 2
#define CONTROL_RADIOBUTTON1    1
#define CONTROL_RADIOBUTTON2    2       // It is OK for this to have the same ID as CONTROL_RADIOBUTTONLIST,
										// because it is a child control under CONTROL_RADIOBUTTONLIST

// IDs for the Task Dialog Buttons
#define IDC_BASICFILEOPEN                       100
#define IDC_ADDITEMSTOCUSTOMPLACES              101
#define IDC_ADDCUSTOMCONTROLS                   102
#define IDC_SETDEFAULTVALUESFORPROPERTIES       103
#define IDC_WRITEPROPERTIESUSINGHANDLERS        104
#define IDC_WRITEPROPERTIESWITHOUTUSINGHANDLERS 105

/* File Dialog Event Handler *****************************************************************************************************/

class CDialogEventHandler : public IFileDialogEvents,
	public IFileDialogControlEvents
{
public:
	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; };
	IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; };
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
	IFACEMETHODIMP OnTypeChange(IFileDialog* pfd);
	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; };

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem);
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; };
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; };
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; };

	CDialogEventHandler() : _cRef(1) { };
private:
	~CDialogEventHandler() { };
	long _cRef;
};

// IFileDialogEvents methods
// This method gets called when the file-type is changed (combo-box selection changes).
// For sample sake, let's react to this event by changing the properties show.
HRESULT CDialogEventHandler::OnTypeChange(IFileDialog* pfd)
{
	IFileSaveDialog* pfsd;
	HRESULT hr = pfd->QueryInterface(&pfsd);
	if (SUCCEEDED(hr))
	{
		UINT uIndex;
		hr = pfsd->GetFileTypeIndex(&uIndex);   // index of current file-type
		if (SUCCEEDED(hr))
		{
			IPropertyDescriptionList* pdl = NULL;

			switch (uIndex)
			{
			case INDEX_EXE:
				// When .doc is selected, let's ask for some arbitrary property, say Title.
				hr = PSGetPropertyDescriptionListFromString(L"prop:System.Title", IID_PPV_ARGS(&pdl));
				if (SUCCEEDED(hr))
				{
					// FALSE as second param == do not show default properties.
					hr = pfsd->SetCollectedProperties(pdl, FALSE);
					pdl->Release();
				}
				break;

			case INDEX_WEBPAGE:
				// When .html is selected, let's ask for some other arbitrary property, say Keywords.
				hr = PSGetPropertyDescriptionListFromString(L"prop:System.Keywords", IID_PPV_ARGS(&pdl));
				if (SUCCEEDED(hr))
				{
					// FALSE as second param == do not show default properties.
					hr = pfsd->SetCollectedProperties(pdl, FALSE);
					pdl->Release();
				}
				break;

			case INDEX_TEXTDOC:
				// When .txt is selected, let's ask for some other arbitrary property, say Author.
				hr = PSGetPropertyDescriptionListFromString(L"prop:System.Author", IID_PPV_ARGS(&pdl));
				if (SUCCEEDED(hr))
				{
					// TRUE as second param == show default properties as well, but show Author property first in list.
					hr = pfsd->SetCollectedProperties(pdl, TRUE);
					pdl->Release();
				}
				break;
			}
		}
		pfsd->Release();
	}
	return hr;
}

// IFileDialogControlEvents
// This method gets called when an dialog control item selection happens (radio-button selection. etc).
// For sample sake, let's react to this event by changing the dialog title.
HRESULT CDialogEventHandler::OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem)
{
	IFileDialog* pfd = NULL;
	HRESULT hr = pfdc->QueryInterface(&pfd);
	if (SUCCEEDED(hr))
	{
		if (dwIDCtl == CONTROL_RADIOBUTTONLIST)
		{
			switch (dwIDItem)
			{
			case CONTROL_RADIOBUTTON1:
				hr = pfd->SetTitle(L"Longhorn Dialog");
				break;

			case CONTROL_RADIOBUTTON2:
				hr = pfd->SetTitle(L"Vista Dialog");
				break;
			}
		}
		pfd->Release();
	}
	return hr;
}

// Instance creation helper
HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void** ppv)
{
	*ppv = NULL;
	CDialogEventHandler* pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}


/* Common File Dialog Snippets ***************************************************************************************************/

// This code snippet demonstrates how to work with the common file dialog interface
HRESULT BasicFileOpen()
{
	// CoCreate the File Open Dialog object.
	IFileDialog* pfd = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	if (FAILED(hr))
	{
		return hr;
	}
	// Create an event handling object, and hook it up to the dialog.
	IFileDialogEvents* pfde = NULL;
	hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
	if (FAILED(hr))
	{
		pfd->Release();
		return hr;
	}

	// Hook up the event handler.
	DWORD dwCookie;
	hr = pfd->Advise(pfde, &dwCookie);
	if (SUCCEEDED(hr))
	{
		// Set the options on the dialog.
		DWORD dwFlags;

		IShellItem* psi;
		SHGetKnownFolderItem(FOLDERID_ProgramFiles,
			KF_FLAG_DEFAULT, nullptr, IID_PPV_ARGS(&psi));
		pfd->SetDefaultFolder(psi);
		psi->Release();

		// Before setting, always get the options first in order not to override existing options.
		hr = pfd->GetOptions(&dwFlags);
		if (SUCCEEDED(hr))
		{
			// In this case, get shell items only for file system items.
			hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
			if (SUCCEEDED(hr))
			{
				// Set the file types to display only. Notice that, this is a 1-based array.
				hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
				if (SUCCEEDED(hr))
				{
					// Set the selected file type index to Word Docs for this example.
					hr = pfd->SetFileTypeIndex(INDEX_EXE);
					if (SUCCEEDED(hr))
					{
						// Set the default extension to be ".doc" file.
						hr = pfd->SetDefaultExtension(L"exe");
						if (SUCCEEDED(hr))
						{
							// Show the dialog
							hr = pfd->Show(NULL);
							if (SUCCEEDED(hr))
							{
								// Obtain the result, once the user clicks the 'Open' button.
								// The result is an IShellItem object.
								IShellItem* psiResult;
								hr = pfd->GetResult(&psiResult);
								if (SUCCEEDED(hr))
								{
									// We are just going to print out the name of the file for sample sake.
									PWSTR pszFilePath = NULL;
									hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
									if (SUCCEEDED(hr))
									{
										wprintf(L"%s", pszFilePath);
										CoTaskMemFree(pszFilePath);
									}
									psiResult->Release();
								}
							}
						}
					}
				}
			}
		}
		// Unhook the event handler.
		pfd->Unadvise(dwCookie);
	}

	pfde->Release();

	pfd->Release();

	return hr;
}

#pragma warning (disable : 4996)
#if 0
void logMessage(const char* fmt, ...)
{


	va_list argptr;
	char msg[1024];
	va_start(argptr, fmt);
	vsprintf_s(msg, 1024, fmt, argptr);
	va_end(argptr);

	char* logPath = "vcam.log";

	static FILE* logf = NULL;

	if (logf != NULL)
	{
		fprintf(logf, "%s\n", msg);
		fflush(logf);
	}
	else
	{
		logf = fopen(logPath, "a");
	}

	//fclose(pFile);

}
#endif

void play(char* path);
// Application entry point
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
	{
		return -1;
	}


	BasicFileOpen();

	CoUninitialize();

	return 0;
}
