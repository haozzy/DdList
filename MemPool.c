
/*
									Mempool Structure Diagram
._______________________________________________________________________________________________.
|  |  |//|  |  |  |//|//|  |  |  |  |  |  |  |//|//|//|  |//|  |  |//|  |  |  |//|  |  |  |  |  | BLOCK0: CHUCK SIZE = MP_BLK_SIZE / 32 == MP_ALLOC_MIM
|-----------------------------------------------------------------------------------------------|
|           |///////////|           |           |           |///////////|///////////|           | BLOCK1: CHUCK SIZE = MP_BLK_SIZE / 8
|-----------------------------------------------------------------------------------------------|
|                       |///////////////////////|                       |                       | BLOCK2: CHUCK SIZE = MP_BLK_SIZE / 4
|-----------------------------------------------------------------------------------------------|
|                                               |///////////////////////////////////////////////| BLOCK3: CHUCK SIZE = MP_BLK_SIZE / 2
|-----------------------------------------------------------------------------------------------|
|                                        ALLOCED                                                | BLOCK4: CHUCK SIZE = MP_BLK_SIZE / 1  == MP_ALLOC_MAX
|-----------------------------------------------------------------------------------------------|
|////////////////////////////////////////FREE///////////////////////////////////////////////////| BLOCK5: UNALLOC
+-----------------------------------------------------------------------------------------------+
*/

#define MP_BLK_SIZE			(1024)
#define MP_BLK_NR			(16)

#define MP_ALLOC_MAX		(MP_BLK_SIZE)
#define MP_ALLOC_MIM		(MP_BLK_SIZE / 32)

#define SIZE_ALIGN(s, l)	(((l) > (s)) ? (l) : SIZE_ALIGN(s, l * 2))
#define MP_SIZE_ALIGN(s)	SIZE_ALIGN(s, MP_ALLOC_MIM)
static unsigned char  gs_aaucMemPool[MP_BLK_NR][MP_BLK_SIZE] __align(32);
static unsigned char *gsc_pucBeginAddr = &gs_aaucMemPool;
static unsigned char *gsc_pucEndAddr   = &gs_aaucMemPool[MP_BLK_NR][0];
static unsigned short gs_ausBlkAllocSize[MP_BLK_NR];
static unsigned int   gs_auiBitMap[MP_BLK_NR];

uint32 MpInit(void)
{
	memset(&auiBitMap, 0, sizeof(auiBitMap));
	memset(&gs_ausBlkAllocSize, 0, sizeof(gs_ausBlkAllocSize));
}

#define MP_ASSERT(expr) 

unsigned short MpSizeAlign(uint32 uiSize)
{
	unsigned short usSize;

	usSize = MP_ALLOC_MIM;
	while(usSize < uiSize) usSize <<= 1;

	return usSize;
}

#define BBM(s) (s == MP_BLK_SIZE) ? 0x01 : 0

void * MpAlloc(unsigned int uiSize)
{
	int b, c;
	unsigned short usSize;
	
	if(0 == uiSize
	|| MP_BLK_SIZE < uiSize)
	{
		//MP_ASSERT(1);
		return NULL;
	}

	usSize = MpSizeAlign(uiSize);

	for(b = 0; MP_BLK_NR > b; b++)
	{
		if(gs_auiBitMap[b] == 0)
		{	// 未分配块，直接使用 
			gs_ausBlkAllocSize[b] = usSize;
		}
		else if((gs_ausBlkAllocSize[b] != usSize)			/* 已分配块，但是大小不一致 */
		     || (gs_auiBitMap[b] == BM_BLK_MAP(usSize)))	/* 块已分配完   */
		{	
			continue;
		}

		for(c = 0; MP_BLK_NR > c; c++)
		{
			if(!(gs_auiBitMap[b] & (0x01u << c)))
			{
				gs_auiBitMap[b] |= (0x01u << c);
				return (void *)((unsigned char *)&gs_aaucMemPool[b] + (c * usSize));
			}
		}
	}
	
	return NULL;	
}

void * MpFree(void *pvMem)
{
	int b, c;
	
	MP_ASSERT(gsc_pucBeginAddr > pvMem);
	MP_ASSERT(gsc_pucEndAddr <= pvMem);

	b = ((char *)pvMem - gsc_pucBeginAddr) / MP_BLK_SIZE;
	c = (((char *)pvMem - gsc_pucBeginAddr) % MP_BLK_SIZE) / gs_ausBlkAllocSize[b];
	MP_ASSERT(!(gs_auiBitMap[b] & (0x01u << c)));	// free a free mem, caller has bug
	gs_auiBitMap[b] &= ~(0x01u << c);

	return NULL;
}

