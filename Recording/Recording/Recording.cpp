// Recording.cpp : Defines the entry point for the application.
//


#include"stdafx.h"
#include"resource..h"
#include <vector>
#include <fstream> 
#include <cstdlib>                /*  Includes the Standard C library header <stdlib.h> and adds the associated names to the std namespace.                       */
#include <iostream>	     
#include <CommDlg.h>              /*  Common Dialog Boxes.                                                                                                        */
#include <MMSystem.h>             /*  Many functions working with voice are in mmSystem.dll, which is in Windows\System\ directory and it's in MMSystem.h header. */
#pragma comment(lib, "winmm.lib") /*  Tells the linker to add the 'winmm.lib' library.*/
using namespace std;

LPWSTR  app_name = L"*******Recorder********";
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM             ); /* Window Procedure. */
int     readSample              (int   number  , bool      leftchannel  );
void    SaveWavFile             (char *FileName, PWAVEHDR  WaveHeader   ); 
void    Wav                     (char *c       , HWND      hWnd         );

int        number, length, byte_samp, byte_sec, bit_samp                 ;
bool       PLAY       = FALSE                                            ;
bool       mono       = TRUE                                             ;
FILE*      stream                                                        ;
const  int NUMPTS     = 11025 * 10                                       ;
static int sampleRate = 11025                                            ;

// ************************************** Windows Main Function  .   **************************************************************************************
// ************************************** Here starts our program.   *************************************************************************************

int WINAPI WinMain(
	HINSTANCE hThisInstance,                                            /* Pointer to our program , each program has its own id-32 bit number identifying the instance of our program within the OS environment. */
	HINSTANCE hPrevInstance,                                            /* Pointer to the previous program.                       */
	LPSTR     lpszArgument,                                             /* Contains the command-line arguments as an ANSI string. */
	int       nCmdShow)                                                 /* Style of our window.                                   */

{
	MSG              messages  ;                                        /* Here messages to the application are saved.            */
	HWND             win_handle;                                        /* This is the handle for our window.                     */
	WNDCLASSEX       win_class ;                                        /* Contains window class information.                     */


	win_class.cbSize        = sizeof(WNDCLASSEX);                       /*  The size, in bytes, of this structure.*/
	win_class.style         = CS_DBLCLKS;                               /*  Catch double-clicks.                                 */
	win_class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);          /*  Handle to an icon resource.                          */
	win_class.hCursor       = LoadCursor(NULL, IDC_HAND);               /*  This member must be a handle to a cursor resource,(IDC_ARROW-Standard arrow).  */
	win_class.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);          /*  A handle to a small icon that is associated with the window class,IDI_APPLICATION-Default application icon.*/
	win_class.hInstance     = hThisInstance;                            /*  A handle to the instance that contains the window procedure for the class.*/
	win_class.cbClsExtra    = 0;                                        /*  No extra bytes after the window class.              */
	win_class.cbWndExtra    = 0;                                        /*  Structure or the window instance .                  */
	win_class.lpfnWndProc   = WindowProcedure;                          /*  This function is called by windows(A pointer to the window procedure).         */
	win_class.lpszMenuName  = NULL;                                     /*  The resource name of the class menu, as the name appears in the resource file. */
	win_class.lpszClassName = app_name                       ;           /*  String, that specifies the window class name.       */
	win_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 2) ;           /*  Use Windows's default colour as the background of the window.                  */

	if (!RegisterClassEx(&win_class))
	{

		return FALSE;
	}


	/* The class is registered, let's create the program */

	win_handle = CreateWindowEx(
		0,                                                           /* Extended possibilites for variation.    */
		app_name,                                                    /* Classname                               */
		app_name,                                                    /* Title Text                              */
		WS_SYSMENU | WS_MAXIMIZE| WS_MAXIMIZEBOX | WS_MINIMIZEBOX,   /* WS_SYSMENU(window menu in title bar).   */
		CW_USEDEFAULT,                                               /* Windows decides the position            */
		CW_USEDEFAULT,                                               /* Where the window ends up on the screen  */
		1300,                                                        /* The programs width.                     */
		900,                                                         /* Height in pixels.                       */
		HWND_DESKTOP,                                                /* The window is a child-window to desktop.*/
		NULL,                                                        /* Use class menu.                         */
		hThisInstance,                                               /* Program Instance handler.               */
		NULL                                                         /* An optional value passed to the window during the WM_CREATE message, (NULL- No Window Creation data,no additional data is needed. ). */
	);	

	if (win_handle == NULL)
	{
		MessageBox(win_handle, TEXT("error "), TEXT("Error message"), MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	ShowWindow(win_handle, nCmdShow);              /* Make the window visible on the screen.                                */

 /* Message Loop */
	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage (&messages);              /* Translate virtual-key messages into character messages.                 */
		DispatchMessage  (&messages);              /* Send message to WindowProcedure                                         */
	}
	    return messages.wParam;                    /* At the end of the application, the exit code is returned to the system. */

}


//************************************ Function that receives messages ********************************************************************************
LRESULT CALLBACK WindowProcedure(HWND win_handle, UINT message, WPARAM wParam, LPARAM lParam)
{

	static HWND         RecButton;
	static HWND         PlyButton;
	static HWND         StpButton;
	static HPEN         Pen;
	static BOOL         recording, playing, ending, terminating;
	static DWORD        data_length, repetitions = 1;
	static TCHAR        error[] = TEXT("Error allocating memory!");
	static PBYTE        buffer1, buffer2, save_buffer, new_buffer; /* pointer to the byte.                                                              */
	static HWAVEIN      wave_in;
	static HWAVEOUT     wave_out;
	static PWAVEHDR     wave_header1, wave_header2;                /* The WAVEHDR structure defines the header used to identify a waveform-audio buffer.*/
	static WAVEFORMATEX wave;
	HDC                 device_context_handle;
	POINT               point[NUM];                                /* Defines the x- and y- coordinates of a point.                                     */
	BOOL                fSuccess = FALSE;
	/*  Handle messages  */
	switch (message)
	{

	case WM_CREATE:  /*Sent when an application requests that a window be created by calling the CreateWindowEx or CreateWindow function*/
		RecButton = CreateWindow(TEXT("button"), TEXT("RECORD"), WS_VISIBLE | WS_CHILD, 100, 500, 200, 60, win_handle, (HMENU)IDC_RECORD, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		PlyButton = CreateWindow(TEXT("button"), TEXT("PLAY"  ), WS_VISIBLE | WS_CHILD, 500, 500, 200, 60, win_handle, (HMENU)IDC_PLAY  , ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		StpButton = CreateWindow(TEXT("button"), TEXT("STOP"  ), WS_VISIBLE | WS_CHILD, 900, 500, 200, 60, win_handle, (HMENU)IDC_STOP  , ((LPCREATESTRUCT)lParam)->hInstance, NULL);

		if (!RecButton && !PlyButton && !StpButton)
		{
			MessageBox(win_handle, TEXT("buttons haven't been created "), TEXT("Error message"), MB_ICONEXCLAMATION | MB_OK);
			return FALSE;
		}

		EnableWindow(PlyButton, FALSE);                                       /* PlyButton is disabled ( at first).                                */
		EnableWindow(StpButton, FALSE);                                       /* StpButton is disabled ( at first).                                */

		wave_header1 = reinterpret_cast <PWAVEHDR> (malloc(sizeof(WAVEHDR))); /* Defines the header used to identify a waveform-audio buffer.      */
		wave_header2 = reinterpret_cast <PWAVEHDR> (malloc(sizeof(WAVEHDR)));
		save_buffer  = reinterpret_cast <PBYTE>    (malloc(1              ));
		return 0;




	case WM_PAINT:                                                       /* Is sent when the system or another application makes a request to paint a portion of an application's window. */
		PAINTSTRUCT     paint_struct;
		device_context_handle = BeginPaint(win_handle, &paint_struct);   /* Prepares the specified window for painting and fills a PAINTSTRUCT structure with information about the painting. */
		if (device_context_handle == INVALID_HANDLE_VALUE)
		{
			MessageBox(win_handle, TEXT(" INVALID device_context_handle"), TEXT("Error message"), MB_ICONEXCLAMATION | MB_OK);
			return FALSE;
		}

		 
		RECT rect;                                                      /* The RECT structure defines the coordinates of the upper-left and lower-right corners of a rectangle.*/
		rect.top    = 50  ;
		rect.left   = 200 ;
		rect.right  = 850 ;
		rect.bottom = 480 ;


		if (!FillRect(device_context_handle, &rect, (HBRUSH)(COLOR_WINDOW + 2)))  /* Fills the rectangle. */
		{

			MessageBox(win_handle, TEXT(" Failed to fill the rectangle!!!!"), TEXT("Error message"), MB_ICONEXCLAMATION | MB_OK);
			return FALSE;

		}


		if (PLAY == TRUE)
		{
			FillRect(device_context_handle, &rect, (HBRUSH)(COLOR_WINDOW + 3));
			Pen = CreatePen(PS_SOLID, 3, RGB(0, 200, 0));                          /* Creates a logical pen .*/
			if (!Pen)

			{
				MessageBox(win_handle, TEXT(" pen hasn't been created!!!! "), TEXT("Error message"), MB_ICONEXCLAMATION | MB_OK);
				return FALSE;
			}



			SelectObject    (device_context_handle, Pen                 );
			SetMapMode      (device_context_handle, MM_ISOTROPIC        );
			SetWindowExtEx  (device_context_handle, 400, 100, NULL      ); /* Sets the horizontal and vertical extents of the window for a device context   */
			SetViewportExtEx(device_context_handle, 300, 100, NULL      ); /* Sets the horizontal and vertical extents of the viewport for a device context */
			SetViewportOrgEx(device_context_handle, 200, 100, NULL      ); /* Specifies which device point maps to the window origin (0,0).                 */



			int i      = 1    ;
			int num    = 60000;
			int sample = 0    ;

			sample = readSample(i, TRUE);

			point[i].x = i / 20;
			point[i].y = (int)((sample)*1.5);
			MoveToEx(device_context_handle, point[i].x, point[i].y, NULL); /* Updates the current position to the specified point.*/


			while (i < num && sample != (int)0xefffffff)
			{
				// scale the sample

				point[i].x = i / 20;
				point[i].y = (int)((sample)*1.5);
				LineTo(device_context_handle, point[i].x, point[i].y);    /* Draws a line from the current position up to, but not including, the specified point.*/
				i++;
				sample = readSample(i, TRUE);
			}
		}


			DeleteObject(Pen                      );
			DeleteDC    (device_context_handle    );
			EndPaint    (win_handle, &paint_struct);                    /* Marks the end of painting in the specified window */

		    return 0;



	case MM_WIM_OPEN:                                                  /* Is sent to a window when a waveform-audio input device is opened.*/
		// Shrink down the save buffer
		save_buffer = reinterpret_cast <PBYTE>(realloc(save_buffer, 1));

		// Add the buffers
		waveInAddBuffer(                                               /* Sends an input buffer to the given waveform-audio input device. */
			wave_in,                                                   /* Handle to the waveform-audio input device.                      */
			wave_header1,                                              /* Pointer to a WAVEHDR structure that identifies the buffer.      */
			sizeof(WAVEHDR));                                          /* Size, in bytes, of the WAVEHDR structure.                       */
		waveInAddBuffer(wave_in, wave_header2, sizeof(WAVEHDR));

		// Begin sampling
		recording   = TRUE  ;
		ending      = FALSE ;
		data_length = 0     ;
		waveInStart(wave_in);                                         /* Starts input on the given waveform-audio input device.           */
		return        TRUE  ;




	case MM_WIM_DATA:   /*  is sent to a window when waveform-audio data is present in the input buffer and the buffer is being returned to the application. */
	/* Reallocate save buffer memory. */

		new_buffer = reinterpret_cast <PBYTE> (realloc(save_buffer, data_length + ((PWAVEHDR)lParam)->dwBytesRecorded));
		if (new_buffer == NULL)
		{
			waveInClose(wave_in);                                          /* Closes the given waveform-audio input device.                     */
			MessageBox(
				win_handle,                                                /* Handle to the owner window of the message box to be created.      */
				error,                                                     /* The message to be displayed.                                      */
				app_name,                                                  /* The dialog box title.                                             */
				MB_ICONEXCLAMATION | MB_OK);                               /* (MB_ICONEXCLAMATION)An exclamation-point icon.                    */
			return TRUE;
		}



		save_buffer = new_buffer;
		CopyMemory(                                                        /* Copies a block of memory from one location to another.              */
			save_buffer + data_length,                                     /* A pointer to the starting address of the copied block's destination.*/
			((PWAVEHDR)lParam)->lpData,                                    /* A pointer to the starting address of the block of memory to copy.   */
			((PWAVEHDR)lParam)->dwBytesRecorded);                          /* The size of the block of memory to copy, in bytes.                  */


		data_length += ((PWAVEHDR)lParam)->dwBytesRecorded;
		if (ending)
		{
			waveInClose(wave_in);
			return TRUE;
		}


		waveInAddBuffer(wave_in, (PWAVEHDR)lParam, sizeof(WAVEHDR));     /* Send out a new buffer.                                             */
		return TRUE;



	case MM_WIM_CLOSE:                                                  /* Is sent to a window when a waveform-audio input device is closed. */
	/* Free the buffer memory. */
		waveInUnprepareHeader(                                          /* cleans up the preparation performed by the waveInPrepareHeader function. This function must be called after the device driver fills a buffer and returns it to the application. You must call this function before freeing the buffer. */
			wave_in,                                                    /* Handle to the waveform-audio input device. */
			wave_header1,                                               /* Pointer to a WAVEHDR structure identifying the buffer to be cleaned up. */
			sizeof(WAVEHDR));                                           /* Size, in bytes, of the WAVEHDR structure. */


		waveInUnprepareHeader(
			wave_in,
			wave_header2,
			sizeof(WAVEHDR));

		free(buffer1);                                                   /* Deallocates or frees a memory block. */
		free(buffer2);                                                   /* Deallocates or frees a memory block. */


		if (data_length > 0)
		{
			EnableWindow(PlyButton, TRUE);
		}

		recording = FALSE;
		
		return TRUE;


	case MM_WOM_OPEN:                    /* This message is sent to a window when the given waveform-audio output device is opened. */
										 /* Set up header. */

		wave_header1->lpData          = reinterpret_cast <CHAR*>(save_buffer);    /* Pointer to the waveform buffer.  */
		wave_header1->dwUser          = 0;                                        /* User data. */
		wave_header1->lpNext          = NULL;                                     /* Reserved. */
		wave_header1->dwFlags         = WHDR_BEGINLOOP | WHDR_ENDLOOP;            /* (WHDR_BEGINLOOP-This buffer is the first buffer in a loop. This flag is used only with output buffers.)
																				  (WHDR_ENDLOOP-This buffer is the last buffer in a loop. This flag is used only with output buffers.) */
		wave_header1->dwLoops         = repetitions;                              /* Number of times to play the loop. This member is used only with output buffers.*/
		wave_header1->reserved        = 0;                                        /* Reserved. */
		wave_header1->dwBufferLength  = data_length;                              /* Length, in bytes, of the buffer. */
		wave_header1->dwBytesRecorded = 0;                                        /* When the header is used in input, specifies how much data is in the buffer. */

	   /*  Prepare and write */
		waveOutPrepareHeader(                                                 /* prepares a waveform-audio data block for playback.*/
			wave_out,                                                         /* Handle to the waveform-audio output device. */
			wave_header1,                                                     /* Pointer to a WAVEHDR structure that identifies the data block to be prepared. */
			sizeof(WAVEHDR));                                                 /* Size, in bytes, of the WAVEHDR structure. */

		waveOutWrite(                                                         /* sends a data block to the given waveform-audio output device. */
			wave_out,                                                         /* Handle to the waveform-audio output device. */
			wave_header1,                                                     /* Pointer to a WAVEHDR structure containing information about the data block. */
			sizeof(WAVEHDR));                                                 /* Size, in bytes, of the WAVEHDR structure. */


		ending  = FALSE ;
		playing = TRUE  ;
		return    TRUE  ;


	case MM_WOM_DONE:                                                    /* Is sent to a window when the given output buffer is being returned to the application. Buffers are returned to the application when they have been played, or as the result of a call to the waveOutReset function. */
		waveOutUnprepareHeader(                                          /* Cleans up the preparation performed by the waveOutPrepareHeader function. This function must be called after the device driver is finished with a data block. You must call this function before freeing the buffer. */
			wave_out,                                                    /* Handle to the waveform-audio output device. */
			wave_header1,                                                /* Pointer to a WAVEHDR structure identifying the data block to be cleaned up. */
			sizeof(WAVEHDR));                                            /* Size, in bytes, of the WAVEHDR structure. */




		waveOutClose(wave_out);                                          /* Closes the given waveform-audio output device. */

		EnableWindow(                                                    /* Enables or disables mouse and keyboard input to the specified window or control. When input is disabled, the window does not receive input such as mouse clicks and key presses. When input is enabled, the window receives all input */
			PlyButton,                                                   /* A handle to the window to be enabled or disabled.  */
			TRUE);                                                       /* TRUE, the window is enabled. If the parameter is FALSE, the window is disabled. */
		return TRUE;



	case MM_WOM_CLOSE: /*  Is sent to a window when a waveform-audio output device is closed. The device handle is no longer valid after this message has been sent. */
		repetitions = 1;
		playing = FALSE;
		return    TRUE ;




	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_RECORD:
		{
			EnableWindow(RecButton, FALSE);
			EnableWindow(PlyButton, FALSE);
			EnableWindow(StpButton, TRUE );

			waveOutReset(wave_out); /* Stops playback on the given waveform-audio output device and resets the current position to zero */
			waveInReset(wave_in); /* Stops input on the given waveform-audio input device and resets the current position to zero.    */

			buffer1 = reinterpret_cast <PBYTE> (malloc(INP_BUFFER_SIZE));
			buffer2 = reinterpret_cast <PBYTE> (malloc(INP_BUFFER_SIZE));

			if (!buffer1 || !buffer2)
			{
				if (buffer1) free(buffer1);
				if (buffer2) free(buffer2);
				MessageBox(win_handle, error, app_name, MB_ICONEXCLAMATION | MB_OK);
				return TRUE;

			}

			// Open waveform audio for input

			wave.cbSize          = 0               ; /* Size, in bytes, of extra format information appended to the end of the WAVEFORMATEX structure. */
			wave.nChannels = 1;                      /* Number of channels in the waveform-audio data.*/
			wave.wFormatTag      = WAVE_FORMAT_PCM; /* Waveform-audio format type. */
			wave.nBlockAlign     = 1    ;           /* Block alignment, in bytes ((nChannels × wBitsPerSample) / 8. )*/
			wave.wBitsPerSample  = 8    ;           /* Bits per sample for the wFormatTag format type*/
			wave.nSamplesPerSec  = 11025;           /* Sample rate, in samples per second (hertz)*/
			wave.nAvgBytesPerSec = 11025;           /* Data-transfer rate, in bytes per second(nSamplesPerSec × nBlockAlign. ).  */

			if (waveInOpen(&wave_in, WAVE_MAPPER, &wave, (DWORD)win_handle, 0, CALLBACK_WINDOW))  /* Opens the given waveform-audio input device for recording.*/
			{
				free(buffer1);
				free(buffer2);
			}

			/*  Set up headers and prepare them */
			wave_header1->lpNext          = NULL;
			wave_header1->lpData          = reinterpret_cast <CHAR*>(buffer1);
			wave_header1->dwUser          = 0;
			wave_header1->dwFlags         = 0;
			wave_header1->dwLoops         = 0;
			wave_header1->reserved        = 0;
			wave_header1->dwBufferLength  = INP_BUFFER_SIZE;
			wave_header1->dwBytesRecorded = 0;

			waveInPrepareHeader(wave_in, wave_header1, sizeof(WAVEHDR)); /* Prepares a buffer for waveform-audio input. */

			wave_header2->lpData          = reinterpret_cast <CHAR*>(buffer2);
			wave_header2->lpNext          = NULL;
			wave_header2->dwUser          = 0;
			wave_header2->dwFlags         = 0;
			wave_header2->dwLoops         = 1;
			wave_header2->reserved        = 0;
			wave_header2->dwBufferLength  = INP_BUFFER_SIZE;
			wave_header2->dwBytesRecorded = 0;

			waveInPrepareHeader(wave_in, wave_header2, sizeof(WAVEHDR));
		}
		break;

		case IDC_STOP:
		{

			_fcloseall();                /* Closes all open streams. */
			EnableWindow(RecButton, TRUE );
			EnableWindow(StpButton, FALSE);
			EnableWindow(PlyButton, TRUE );

			ending = TRUE;
			SaveWavFile("temp.wav", wave_header1);
		}



		case IDC_PLAY:
		{
			// play wav file
			playing = TRUE;
			EnableWindow(PlyButton, FALSE);
			wave.cbSize          = 0;               /* Size, in bytes, of extra format information appended to the end of the WAVEFORMATEX structure. */
			wave.nChannels       = 1;               /* Number of channels in the waveform-audio data.                            */
			wave.wFormatTag      = WAVE_FORMAT_PCM; /* Waveform-audio format type.                                               */
			wave.nBlockAlign     = 1;               /* Block alignment, in bytes ((nChannels × wBitsPerSample) / 8. )            */
			wave.wBitsPerSample  = 8;               /* Bits per sample for the wFormatTag format type                            */
			wave.nSamplesPerSec  = 11025;           /* Sample rate, in samples per second (hertz)                                */
			wave.nAvgBytesPerSec = 11025;           /* Data-transfer rate, in bytes per second(nSamplesPerSec × nBlockAlign. ).  */

			
			waveInReset(wave_in  );                 /* Stops input on the given waveform-audio input device                      */
			waveOutReset(wave_out);                 /* Stops playback on the given waveform-audio output device                  */

			if (waveOutOpen(&wave_out, WAVE_MAPPER, &wave,(DWORD)win_handle, 0, CALLBACK_WINDOW)) /*  opens the given waveform-audio output device for playback. */
			{
				MessageBox(win_handle, error, app_name,MB_ICONEXCLAMATION | MB_OK);
			}

			Wav("temp.wav", win_handle);

			RECT rc    ;
			GetClientRect (win_handle, &rc       );        /* Retrieves the coordinates of a window's client area.     */
			PLAY = TRUE;
			InvalidateRect(win_handle, &rc, FALSE);        /* Adds a rectangle to the specified window's update region.*/
		}
		break;

		}

		break;
	case WM_DESTROY:                                     /*Sent when a window is being destroyed. */
		PostQuitMessage(0);                              /* send a WM_QUIT to the message queue */						 
		_fcloseall();

		fSuccess=DeleteFile(TEXT("temp.wav"));

		break;
	default:                      /* for messages that we don't deal with */

		return DefWindowProc(win_handle, message, wParam, lParam); /* Calls the default window procedure to provide default processing for any window messages that an application does not process. */
		
	}

	return 0;

}





//read the temporary wav file 
void Wav(char *c, HWND hWnd)
{
	int         s_rate = 11025;
	char       *filename;
	errno_t     wavfile;

	filename = new char[strlen(c) + 1];
	strcpy_s(filename, strlen(c) + 1, c);


	wavfile = fopen_s(&stream, filename, "r");  /* Opens for reading. If the file does not exist or cannot be found, the fopen_s call fails.*/
	if (stream == NULL)
	{
		MessageBox(hWnd, L"Could not open ", L"Error", MB_OK);
	}
	else

	{

		/* declare a char buff to store some values in */

		char *buff = new char[5];

		buff[4] = '\0';
		// read the first 4 bytes

		fread((void*)buff, 1, 4, stream);

		// the first four bytes should be 'RIFF'

		if (strcmp(buff, "RIFF") == 0)

		{

			// read byte 8,9,10 and 11
			fseek(stream, 4, SEEK_CUR);
			fread((void*)buff, 1, 4, stream);
			// this should read "WAVE"
			if (strcmp((char*)buff, "WAVE") == 0)
			{
				// read byte 12,13,14,15
				fread((void*)buff, 1, 4, stream);
				// this should read "fmt "

				if (strcmp((char*)buff, "fmt ") == 0)
				{
					fseek(stream, 20, SEEK_CUR);
					// final one read byte 36,37,38,39
					fread((void*)buff, 1, 4, stream);
					if (strcmp((char*)buff, "data") == 0)
					{

						// Now we know it is a wav file, rewind the stream
						rewind(stream);  /* Sets the file position to the beginning of the file of the given stream. */
										 // now is it mono or stereo ?

						fseek(stream, 22, SEEK_CUR);
						fread((void *)buff, 1, 2, stream);
						if (buff[0] == 0x02)
						{
							mono = false;
						}
						else
						{
							mono = true;
						}

						/* read the sample rate */

						fread((void *)&s_rate, 1, 4, stream);
						fread((void *)&byte_sec, 1, 4, stream);
						byte_samp = 0;
						fread((void *)&byte_samp, 1, 2, stream);
						bit_samp = 0;
						fread((void *)&bit_samp, 1, 2, stream);
						fseek(stream, 4, SEEK_CUR);
						fread((void *)&length, 1, 4, stream);

					}

				}
			}
		}
		delete buff;
	}
}







void SaveWavFile(char *FileName, PWAVEHDR WaveHeader)
{
	fstream myFile(FileName , fstream::out | fstream::binary);
	int chunksize, pcmsize, NumSamples, subchunk1size;
	int audioFormat   = 1;
	int numChannels   = 1;
	int bitsPerSample = 8;
	NumSamples        = ((long)(NUMPTS / sampleRate) * 1000);
	pcmsize           = sizeof(PCMWAVEFORMAT);
	subchunk1size     = 16;
	int byteRate      = sampleRate*numChannels*bitsPerSample / 8;
	int blockAlign    = numChannels*bitsPerSample / 8;
	int subchunk2size = WaveHeader->dwBufferLength*numChannels;
	chunksize         = (36 + subchunk2size);

	// write the wav file format
	myFile.seekp(0, ios::beg                 );
	myFile.write("RIFF",                4    );                                   /* ChunkID                                                    */
	myFile.write((char*)&chunksize,     4    );                                   /* Chunk size (36 + SubChunk2Size))                           */
	myFile.write("WAVE",                4    );                                   /* Format                                                     */
	myFile.write("fmt ",                4    );                                   /* subchunk1ID                                                */
	myFile.write((char*)&subchunk1size, 4    );                                   /* subchunk1size (16 for PCM)                                 */
	myFile.write((char*)&audioFormat,   2    );                                   /* AudioFormat (1 for PCM)                                    */
	myFile.write((char*)&numChannels,   2    );                                   /* NumChannels                                                */
	myFile.write((char*)&sampleRate,    4    );                                   /* sample rate                                                */
	myFile.write((char*)&byteRate,      4    );                                   /* byte rate (SampleRate * NumChannels * BitsPerSample/8)     */
	myFile.write((char*)&blockAlign,    2    );                                   /* block align (NumChannels * BitsPerSample/8)                */
	myFile.write((char*)&bitsPerSample, 2    );                                   /* bits per sample                                            */
	myFile.write("data",                4    );                                   /* subchunk2ID                                                */
	myFile.write((char*)&subchunk2size, 4    );                                   /* subchunk2size (NumSamples * NumChannels * BitsPerSample/8) */
	myFile.write(WaveHeader->lpData, WaveHeader->dwBufferLength);                 /* data                                                       */
	myFile.close();
}







	//start of wave visualisation process
	int readSample(int number, bool leftchannel)
	{
		/*
		Reads sample number, returns it as an int, if
		this.mono==false we look at the leftchannel bool
		to determine which to return.
		number is in the range [0,length/byte_samp]
		returns 0xefffffff on failure
		*/

		if (number >= 0 && number < length / byte_samp)
		{

			rewind(stream);                /* Go to beginning of the file.                                     */
			int offset = number * 1 + 44;  /* We start reading at sample_number * sample_size + header length. data in the canonical wave format is held there at this point. /

			                               /* Unless this is a stereo file and the rightchannel is requested.  */
			if (!mono && !leftchannel)
			{
				offset += byte_samp / 2;
			}
			

			                                  /* Read this many bytes;       */
			int amount;
			amount = byte_samp;
			fseek(stream, offset, SEEK_CUR);  /* Sets the position in file. */
			short sample = 0;
			fread((void *)&sample, 1, amount, stream); 
			return sample;
		}
		else
		{
			                                 /* Return 0xefffffff if failed */
			return (int)0xefffffff;
		}
	}












