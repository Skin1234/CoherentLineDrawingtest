#pragma once 

#define _CRT_NONSTDC_NO_WARNINGS 
//#define  _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "FileNameSep.h"



typedef enum
{
	MET_CHAR, 
	MET_UCHAR, 
	MET_SHORT,
	MET_USHORT,
	MET_INT, 
	MET_UINT,
	MET_LONG,
	MET_ULONG,
	MET_FLOAT, 
	MET_DOUBLE,
}MET_ValueEnumType;


const int c_nMetaTypes = 10;

class CMetaVolume  
{
public:
	MET_ValueEnumType m_metaType;
	int				  m_nMetaSize;
public:

	char		m_cVolumeFile[256];
	//像素空间
	int			m_nSize[3];		//原始大小
	float       m_fSpc[3];		//各个轴的比例

	int			m_nDims;

	int			m_nChannels;	//channel

	int			m_bChangeZ;
		
	char m_cElementTypes[c_nMetaTypes][256];
	MET_ValueEnumType m_metaTypes[c_nMetaTypes];
	int	m_nTypeSizes[c_nMetaTypes];
	int	m_nMetas;

	int		m_nIndex;	//当前在m_metaTypes表中的索引
	char	m_cElementType[256];		//当前特征字符串


private:
	int GetSize(){return m_nSize[0]*m_nSize[1]*m_nSize[2];}
public:

	CMetaVolume()
	{
		char cMsg[][256]={  
			"MET_UCHAR", 
			"MET_SHORT", "MET_USHORT", 
			"MET_INT", "MET_UINT", 
			"MET_ULONG", 
			"MET_FLOAT", "MET_DOUBLE"};
		MET_ValueEnumType metaType[] = {
				MET_UCHAR, 
				MET_SHORT, MET_USHORT, 
				MET_INT, MET_UINT, 
				MET_ULONG, 
				MET_FLOAT, MET_DOUBLE
			};
		int nSize0[]   ={sizeof(char), 
			sizeof(short), sizeof(short), 
			sizeof(int), sizeof(int), 
			sizeof(long), 
			sizeof(float), sizeof(double)};

		m_nMetas = sizeof(nSize0)/sizeof(int);
		for(int i=0; i<m_nMetas; i++)
		{
			sprintf_s(m_cElementTypes[i], "%s", cMsg[i]);
			m_metaTypes[i] = metaType[i];
			m_nTypeSizes[i] = nSize0[i];
		}

		m_nChannels = 1;
		m_bChangeZ = 0;

		m_nSize[2] = 1;
	}
private:
	unsigned char* ReadRaw(char *cRawFile)
	{

		int nSize, nEleSize;
		int i;
		for(i=0; i<m_nMetas; i++)
		{
			if(!_stricmp(m_cElementTypes[i], m_cElementType))
			{
				nEleSize = m_nTypeSizes[i];
				m_nIndex = i;
				break;
			}
		}
		nSize = GetSize()*m_nChannels*nEleSize;
		unsigned char *pSrc = new unsigned char[nSize];
		int nLevelSize = m_nSize[0]*m_nSize[1]*m_nChannels*nEleSize;
		unsigned char *pTemp = new unsigned char[nLevelSize];

		FILE *fp;
		fopen_s(&fp, cRawFile, "rb");
		int nRead;

		if(m_bChangeZ)
		{//z方向交换顺序
			for(i=0; i<m_nSize[2]; i++)
			{
				fread(pTemp, 1, nLevelSize, fp);

				memcpy(pSrc + (m_nSize[2]-1-i)*nLevelSize, pTemp, nLevelSize);

			}
		}
		else
		{//不交换顺序
			nRead = fread(pSrc, 1, nSize, fp);
		}

		fclose(fp);
		delete[] pTemp;

		return pSrc;
	}

public:
	void* LoadMHD(char *cMHDFileName, int &ndims, int *dims, MET_ValueEnumType &type, bool bChangeZ=0)
	{
		FILE *fp;
		fopen_s(&fp, cMHDFileName, "rt");
		if(!fp)
		{
			return 0;
		}

		m_bChangeZ = bChangeZ;

//		char cDataType[256];
		char cRawFile[256];
		char cTemp[256];
		char *p;

		while(!feof(fp))
		{
			fgets(cTemp, 256, fp);
			p = strstr(cTemp, "=")+1;
			if(strstr(cTemp, "ElementType"))
			{
				sscanf_s(p, "%s", m_cElementType);
			}
			else if(strstr(cTemp, "ElementDataFile"))
			{
				sscanf_s(p, "%s", cRawFile);
			}
			else if(strstr(cTemp, "DimSize"))
			{
				if(m_nDims==3)
					sscanf_s(p, "%d %d %d", &m_nSize[0], &m_nSize[1], &m_nSize[2]);
				else if(m_nDims==2)
					sscanf_s(p, "%d %d", &m_nSize[0], &m_nSize[1]);
			}
			else if(strstr(cTemp, "NDims"))
			{
				int nDims;
				sscanf_s(p, "%d", &m_nDims);

			}
			else if(strstr(cTemp, "ElementSpacing"))
			{
				if(m_nDims==3)
					sscanf_s(p, "%f %f %f", &m_fSpc[0],&m_fSpc[1],&m_fSpc[2]);
				else
					sscanf_s(p, "%f %f", &m_fSpc[0],&m_fSpc[1]);
			}
			else if(strstr(cTemp, "ElementNumberOfChannels"))
			{
				sscanf_s(p, "%d", &m_nChannels);
			}
		}
		fclose(fp);

		CFileNameSep sep0(cMHDFileName);
		CFileNameSep sep(cRawFile);

		char cRawFull[256];
		sprintf_s(cRawFull, "%s%s", sep0.GetDir(), cRawFile);
		unsigned char *pData = NULL;
		pData = ReadRaw(cRawFull);

		ndims = m_nDims;
		dims[0] = m_nSize[0];dims[1] = m_nSize[1];dims[2] = m_nSize[2];

		return pData;
	}
};

/**
sample:
WrtieMhd2D("1.mhd", "1.raw", dim, MET_FLOAT);
*/
inline void WrtieMhd2DHeader(char *cFile, char *cRaw, int *dim, MET_ValueEnumType cType)
{

	char cMsg[][256]=
	{  
		"MET_UCHAR", 
		"MET_SHORT", "MET_USHORT", 
		"MET_INT", "MET_UINT", 
		"MET_ULONG", 
		"MET_FLOAT", "MET_DOUBLE"
	};

	MET_ValueEnumType metaType[] = 
	{
		MET_UCHAR, 
		MET_SHORT, MET_USHORT, 
		MET_INT, MET_UINT, 
		MET_ULONG, 
		MET_FLOAT, MET_DOUBLE
	};

	int nSize0[]   =
	{
		sizeof(char), 
		sizeof(short), sizeof(short), 
		sizeof(int), sizeof(int), 
		sizeof(long), 
		sizeof(float), sizeof(double)
	};

	int index = 0;
	for(int i=0; i<sizeof(nSize0)/sizeof(int); i++)
	{
		if(metaType[i]==cType)
		{
			index = i;
			break;
		}
	}


	FILE *fp;
	fopen_s(&fp, cFile, "w+t");
	fprintf(fp, "ObjectType = Image\n");
	fprintf(fp, "BinaryData = True\n");
	fprintf(fp, "ElementType = %s\n", cMsg[index]);

	fprintf(fp, "ElementNumberOfChannels = 1\n");
	fprintf(fp, "NDims = 2\n");
	fprintf(fp, "DimSize = %d %d\n", dim[0], dim[1]);

	fprintf(fp, "ElementDataFile = %s\n", cRaw);
	fclose(fp);

}


inline void WrtieMhd2D(char *cFile, char *cRaw, int *dim, MET_ValueEnumType cType, const void *pData)
{
	WrtieMhd2DHeader(cFile, cRaw, dim, cType);

	MET_ValueEnumType metaType[] = 
	{
		MET_UCHAR, 
		MET_SHORT, MET_USHORT, 
		MET_INT, MET_UINT, 
		MET_ULONG, 
		MET_FLOAT, MET_DOUBLE
	};

	int nSize0[]   =
	{
		sizeof(char), 
		sizeof(short), sizeof(short), 
		sizeof(int), sizeof(int), 
		sizeof(long), 
		sizeof(float), sizeof(double)
	};

	int index = 0;
	for(int i=0; i<sizeof(nSize0)/sizeof(int); i++)
	{
		if(metaType[i]==cType)
		{
			index = i;
			break;
		}
	}

	CFileNameSep sep(cFile);

	char cRaw0[256];
	sprintf_s(cRaw0, "%s%s", sep.GetDir(), cRaw);

	FILE *fp;
	fopen_s(&fp, cRaw0, "w+b");
	fwrite(pData, nSize0[index], dim[0]*dim[1], fp);
	fclose(fp);

}


inline void WrtieMhd2D(char *cFile, int *dim, MET_ValueEnumType cType, const void *pData)
{
	CFileNameSep sep(cFile);
	char cRaw[256];
	sprintf_s(cRaw, "%s", sep.GetFileName().c_str());

	WrtieMhd2DHeader(cFile, cRaw, dim, cType);

	MET_ValueEnumType metaType[] = 
	{
		MET_UCHAR, 
		MET_SHORT, MET_USHORT, 
		MET_INT, MET_UINT, 
		MET_ULONG, 
		MET_FLOAT, MET_DOUBLE
	};

	int nSize0[]   =
	{
		sizeof(char), 
		sizeof(short), sizeof(short), 
		sizeof(int), sizeof(int), 
		sizeof(long), 
		sizeof(float), sizeof(double)
	};

	int index = 0;
	for(int i=0; i<sizeof(nSize0)/sizeof(int); i++)
	{
		if(metaType[i]==cType)
		{
			index = i;
			break;
		}
	}


	char cRaw0[256];
	sprintf_s(cRaw0, "%s%s", sep.GetDir().c_str(), cRaw);

	FILE *fp;
	fopen_s(&fp, cRaw0, "w+b");
	fwrite(pData, nSize0[index], dim[0]*dim[1], fp);
	fclose(fp);

}
//WrtieMhd2D("some.mhd", 128, 128, MET_DOUBLE, data);
inline void WrtieMhd2D(char *cFile, int w, int h, MET_ValueEnumType cType, const void *pData)
{
	int dim[2] = {w, h};
	WrtieMhd2D(cFile, dim, cType, pData);
}

