#ifndef APRSERVICE_API_H
#define APRSERVICE_API_H

#define APRSERVICE_CALL __cdecl

#if defined(APRSERVICE_WIN32)
	#ifdef APRSERVICE_API
		#define APRSERVICE_EXPORT __declspec(dllexport)
	#else
		#define APRSERVICE_EXPORT __declspec(dllimport)
	#endif
#else
	#define APRSERVICE_EXPORT 
#endif

#endif
