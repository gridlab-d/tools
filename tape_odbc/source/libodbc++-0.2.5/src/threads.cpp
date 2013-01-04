#include <odbc++/threads.h>
#include <odbc++/types.h>

#include <errno.h>

#if defined(WIN32)
#ifdef _DEBUG
#		define	_CRTDBG_MAP_ALLOC
#		include <crtdbg.h>
#include <map>
	//-------------------------------------------------------------------
	typedef std::map<ODBCXX_STRING,int>			StringToIntMap;
	typedef StringToIntMap::iterator			StringToIntMapIter;
	typedef StringToIntMap::value_type		StringToIntMap_Pair;
	typedef std::pair<StringToIntMapIter, bool>	StringToIntMap_InsRet;

	char	filenamebuff[512][512] = {{NULL}};
	int		lastslot = 0;
StringToIntMap	g_ofilenames;
const char*	getFilenamePtr(const char* fname)
{
	const char*	retVal = fname;
	if(fname != NULL)
	{
		StringToIntMapIter foundval = g_ofilenames.find(fname);
		if(foundval == g_ofilenames.end())
		{	
			int currslot = lastslot++;
			StringToIntMap_InsRet ret = 
				g_ofilenames.insert(StringToIntMap_Pair(fname, currslot));
			if(ret.second)
			{
				strcpy(filenamebuff[currslot], fname);
				retVal = filenamebuff[currslot];
			}
		}
		else
		{	
			retVal = filenamebuff[foundval->second];
		}

	}
	return retVal;
}
////////////////////////////////////////////////////////////////////////////////
#ifdef IN_ODBCXX
void*	operator new(size_t n, const char* debFile, int debLine)
{	
	return (char*)_malloc_dbg(n, _NORMAL_BLOCK, getFilenamePtr(debFile), debLine);
}
void	operator delete(void*  p, const char* debFile, int debLine)
{	
	_free_dbg(p, _NORMAL_BLOCK);
}
#endif // IN_ODBCXX
#endif //_DEBUG
#endif // defined(WIN32)
#if defined (ODBCXX_ENABLE_THREADS)

using namespace odbc;
using namespace std;

Mutex::Mutex()
{
#if defined(WIN32)

  InitializeCriticalSection(&mutex_);

#else

  if(pthread_mutex_init(&mutex_,NULL)!=0) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: OS error, mutex initialization failed"), SQLException::scDEFSQLSTATE);
  }

#endif
}

Mutex::~Mutex()
{
#if defined(WIN32)
  DeleteCriticalSection(&mutex_);
#else
  pthread_mutex_destroy(&mutex_);
#endif
}


void Mutex::lock()
{
#if defined(WIN32)

  EnterCriticalSection(&mutex_);

#else

  if(pthread_mutex_lock(&mutex_)!=0) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: OS error, mutex lock failed"), SQLException::scDEFSQLSTATE);
  }

#endif
}


void Mutex::unlock()
{
#if defined(WIN32)

  LeaveCriticalSection(&mutex_);

#else

  if(pthread_mutex_unlock(&mutex_)!=0) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: OS error, mutex unlock failed"), SQLException::scDEFSQLSTATE);
  }

#endif
}


#endif // ODBCXX_ENABLE_THREADS
