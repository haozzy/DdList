	
#define EFD_PAGE_SIZE		128
#define EFD_PAGE_NR_OF_BLK	16
#define EFD_BLK_NR			8

#define EFD_VLD_BLK_NR		(EFD_BLK_NR - 1)
#define EFD_VLD_PAGE_SIZE	(EFD_PAGE_SIZE - 1)

#define EFD_VALID_SIZE		(EFD_VLD_BLK_NR * EFD_PAGE_NR_OF_BLK * EFD_VLD_PAGE_SIZE)

typedef struct
{
	unsigned char	ucLogAddr;
	unsigned char   aucData[EFD_PAGE_SIZE];
} t_PAGE;

typedef struct
{
	t_PAGE	atPage[EFD_PAGE_NR_OF_BLK];
} t_BLK;

typedef struct
{
	t_BLK	atBlk[EFD_BLK_NR];
} t_EF;

#define EFD_ASSERT(expr)	do{if(expr) while(1);}while(0)

void __PortEfdPageRead(unsigned int uiAddr, void *pvData, unsigned int uiSize)
{
}

void __PortEfdPageWrite(unsigned int uiAddr, void *pvData, unsigned int uiSize)
{
}

void __PortEfdBlkErase(unsigned int uiAddr)
{
}

__inline bool __PortEfdWrNeedEr(char cOld, char cNew)
{
	return true;
}

static const t_EF * gsc_ptEmFlash = (t_EF *)0x1c000;
static unsigned char gs_aucEfdPgWrBuf[EFD_VLD_PAGE_SIZE];

#define PHY_PAGE_ADDR_ALIGN(a)
#define LOG_PAGE_ADDR_ALIGN(a)

#define PAGE_ST_DURTY	0x00
#define PAGE_ST_FREE	0xFF

static void PageMark(unsigned int uiAddr, unsigned char ucFlag)
{
	__PortEfdPageWrite(uiAddr, &ucFlag, 1);
}

static int PageDurtyMark(unsigned int uiAddr)
{
	PageMark(uiAddr, PAGE_ST_DURTY);
}

static void GarbgCollect(void)
{
	int b, p;

	for(b = 0; EFD_VLD_BLK_NR > b; b++)
		for(p = 0; EFD_PAGE_NR_OF_BLK > p; p++)
		{
			if(PAGE_ST_FREE == gsc_ptEmFlash->atBlk[b].atPage[p].ucLogAddr)
			{				
				PageMark(LOG_PAGE_ADDR_ALIGN(uiAddr / EFD_VLD_PAGE_SIZE) + 1);

				return (&(gsc_ptEmFlash->atBlk[b].atPage[p].aucData) + (uiAddr % EFD_VLD_PAGE_SIZE))
			}
		}

	return -1;
}

// 待回收块，垃圾页最多
static unsigned char gs_ucGbgPgOfBlkMax = 0;
static unsigned char gs_ucGbgPgOfBlkNr  = 0;

static int LogicPageAlloc(unsigned int uiAddr)
{
	int b, p;
	unsigned char pd;
	
	pd = 0;
	for(b = 0; EFD_BLK_NR > b; b++)
	{	for(p = 0; EFD_PAGE_NR_OF_BLK > p; p++)
		{
			if(PAGE_ST_FREE == gsc_ptEmFlash->atBlk[b].atPage[p].ucLogAddr)
			{				
				PageMark(LOG_PAGE_ADDR_ALIGN(uiAddr / EFD_VLD_PAGE_SIZE) + 1);

				return (&(gsc_ptEmFlash->atBlk[b].atPage[p].aucData) + (uiAddr % EFD_VLD_PAGE_SIZE))
			}
			else if(PAGE_ST_DURTY == gsc_ptEmFlash->atBlk[b].atPage[p].ucLogAddr)
			{
				pd++;
			}
		}
		if(gs_ucGbgPgOfBlkNr < pd)
		{
			gs_ucGbgPgOfBlkNr  = pd;
			gs_ucGbgPgOfBlkMax = b;
		}
	}

	// 待回收块已经找到，下一步重点是什么时候回收
	GarbgCollect();

	return -1;
}

static int LogicPageFind(unsigned int uiAddr)
{
	int b, p;

	for(b = 0; EFD_BLK_NR > b; b++)
		for(p = 0; EFD_PAGE_NR_OF_BLK > p; p++)
		{
			if((uiAddr / EFD_VLD_PAGE_SIZE) == (gsc_ptEmFlash->atBlk[b].atPage[p].ucLogAddr) - 1)
			{				
				PageMark(LOG_PAGE_ADDR_ALIGN(uiAddr));

				return (&(gsc_ptEmFlash->atBlk[b].atPage[p].aucData) + (uiAddr % EFD_VLD_PAGE_SIZE))
			}
		}

	return -1;
}

static int LogicPageWrite(unsigned int uiAddr, void *pvData, unsigned int uiSize)
{
	int i, iAddr;	
	unsigned char *pucOut;
	bool bErFlag;

	pucOut = (char *)pvData;
	if(((uiAddr % EFD_VLD_PAGE_SIZE) + uiSize) > EFD_VLD_PAGE_SIZE)
	{
		uiSize -= (uiAddr % EFD_VLD_PAGE_SIZE) + uiSize - EFD_VLD_PAGE_SIZE;
	}
	
	bErFlag = 0;
	iAddr = LogicPageFind(uiAddr);
	if(0 > iAddr);
	{
		iAddr = LogicPageAlloc(uiAddr);
	}
	else
	{
		__PortEfdPageRead(iAddr, gs_aucEfdPgWrBuf, uiSize);
		for(i = 0; uiSize > i; i++)
		{
			if(__PortEfdWrNeedEr(gs_aucEfdPgWrBuf[i], pucOut))
			{
				bErFlag = true;
				break;
			}
		}
	}
	
	if(bErFlag)
	{
		__PortEfdPageRead(iAddr - (uiAddr % EFD_VLD_PAGE_SIZE), gs_aucEfdPgWrBuf, EFD_VLD_PAGE_SIZE);
		memcpy(&gs_aucEfdPgWrBuf[uiAddr % EFD_VLD_PAGE_SIZE], pvData, uiSize);
		PageDurtyMark(iAddr);
		iAddr = LogicPageAlloc(uiAddr);
		__PortEfdPageWrite(iAddr - (uiAddr % EFD_VLD_PAGE_SIZE), gs_aucEfdPgWrBuf, EFD_VLD_PAGE_SIZE);
	}
	else
	{
		__PortEfdPageWrite(iAddr, pvData, uiSize);
	}

	return uiSize;
}


static int LogicPageRead(unsigned int uiAddr, void *pvData, unsigned int uiSize)
{
	int iAddr;

	if(((uiAddr % EFD_VLD_PAGE_SIZE) + uiSize) > EFD_VLD_PAGE_SIZE)
	{
		uiSize -= (uiAddr % EFD_VLD_PAGE_SIZE) + uiSize - EFD_VLD_PAGE_SIZE;
	}
	
	iAddr = LogicPageFind(uiAddr);
	if(0 > iAddr)
	{	// 未写为空，处理策略返回零值
		memset(pvData, 0, uiSize);
	}
	else
	{
		__PortEfdPageRead(iAddr, pvData, uiSize);
	}

	return uiSize;
}

int EfdWrite(unsigned int uiAddr, void *pvData, unsigned int uiSize)
{
	int iSize;
	unsigned char *pucOut;

	EFD_ASSERT(NULL == pvData);
	EFD_ASSERT((uiAddr + uiSize) > EFD_VALID_SIZE);

	iSize = 0;
	pucOut = (char *)pvData;
	for(iSize = 0; uiSize > iSize; )
	{
		iSize += LogicPageWrite(uiAddr + iSize, pucOut + iSize, uiSize - iSize);
	}

	return iSize;
}

int EfdRead(unsigned int uiAddr, void *pvData, unsigned int uiSize)
{
	int iSize;
	unsigned char *pucOut;

	EFD_ASSERT(NULL == pvData);
	EFD_ASSERT((uiAddr + uiSize) > EFD_VALID_SIZE);

	iSize = 0;
	pucOut = (char *)pvData;
	for(iSize = 0; uiSize > iSize; )
	{
		iSize += LogicPageRead(uiAddr + iSize, pucOut + iSize, uiSize - iSize);
	}

	return iSize;
}

