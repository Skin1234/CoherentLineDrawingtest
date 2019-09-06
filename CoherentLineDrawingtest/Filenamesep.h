#ifndef _FileNameSep_h
#define _FileNameSep_h

//#pragma warning(disable: 4786)

#define _CRT_NONSTDC_NO_WARNINGS 

#include <vector>
#include <algorithm>

using namespace std;



/*
#include "FileNameSep.h"
*/

//the directory's name has a '\' at it's end.
/*******************************************************
目录结尾:
\Test\;c:\;\
扩展名：
mhd,txt
********************************************************/

class CFileNameSep
{
public:
	typedef vector<string> VString;
	VString	m_vDir;
private:
	string m_cFileName;
	string m_cDir;
	string m_cFile;
	string m_cExt;

	void ChangeBackslash(string &str)
	{
		for (unsigned int i = 0; i < str.length(); i++)
		{
			if (str[i] == '/')
				str[i] = '\\';
		}
	}

private:
	void SeparateFile(const string &cFullName, string &cDir, string &cFile, string &cExt)
	{
		unsigned int nPosExt = cFullName.find_last_of('.');
		if (nPosExt < 0)
		{//
			cFile = "";
			cExt = "";
			if (cFullName.back() != '\\')
			{
				cDir = cFullName + "\\";
			}
			return;
		}
		else
		{//
			cExt = cFullName.substr(nPosExt + 1, cFullName.length());
		}

		unsigned int nPosDir = cFullName.find_last_of('\\');
		if (nPosDir < 0)
		{
			cDir = ".\\";
		}
		else
		{
			cDir = cFullName.substr(0, (nPosDir - 0) + 1);
		}

		cFile = cFullName.substr(nPosDir + 1, (nPosExt - (nPosDir + 1)));


	}
private:
	void SegDir()
	{
		string temp = m_cDir;

		while (temp.length())
		{
			int pos = temp.find_first_of('\\');
			string path = temp.substr(0, pos);
			m_vDir.push_back(path);

			temp = temp.substr(pos + 1, temp.length());
		}

	}
public:
	CFileNameSep(const char* cFile)
	{
		m_cFileName = string(cFile);
		ChangeBackslash(m_cFileName);

		SeparateFile(m_cFileName, m_cDir, m_cFile, m_cExt);
		SegDir();
	}

	CFileNameSep(const string &strFile)
	{
		m_cFileName = strFile;
		ChangeBackslash(m_cFileName);

		SeparateFile(m_cFileName, m_cDir, m_cFile, m_cExt);
		SegDir();
	}
	string GetExt()const { return m_cExt; }//扩展名，
	string GetDir()const { return m_cDir; }//路径,带"\\"
	string GetFileName()const { return m_cFile; }//文件名（不带路径和扩展名）
	int	GetDirLevel()const { return (int)m_vDir.size(); }

	string GetDirs(int index)const
	{
		string str = "";
		if (index >= int(m_vDir.size()))
			return str;

		for (int i = 0; i <= index; i++)
		{
			str = str + m_vDir[i] + "\\";
		}
		return str;
	}
};

#endif