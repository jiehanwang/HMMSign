#pragma once
#include <opencv2\opencv.hpp>
#include <cvaux.h>
#include <atlstr.h>
//#include <direct.h>
using namespace cv;

struct HMMParams
{
	int stateNum[32];
	int mixNum[128];
	CvSize dctSize;
	CvSize obsSize;
	CvSize delta;
	int maxiterations;
};

class CSign
{
public:
	CSign(void);
	~CSign(void);

	int m_vectSize;
	bool m_trained;
	CvEHMM* m_hmm;
	CString m_pathname;
	CString per_name;

	void TrainHAMM(HMMParams params, vector<IplImage*> Route);
};

