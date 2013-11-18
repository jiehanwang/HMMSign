#include "StdAfx.h"
#include "Sign.h"


CSign::CSign(void)
{
	m_trained=false;
}


CSign::~CSign(void)
{
}

void CSign::TrainHAMM(HMMParams params, vector<IplImage*> Route)//vector<IplImage*> Route
{
	int vect_len = params.obsSize.height * params.obsSize.width;//vector dimension
	m_vectSize = vect_len;

	m_hmm = cvCreate2DHMM(params.stateNum, params.mixNum, vect_len); //Create a Markov model. state number, 混合元件数

	int num_img = Route.size();//将文件中的图片个数赋值num_imgs
	CvImgObsInfo** obs_info_array = new CvImgObsInfo*[num_img];//Structure for images; 100 for temp

	for (int fileIndex=0; fileIndex<num_img; fileIndex++)
	{
		//IplImage* ipl1 = Route[fileIndex];
		//IplImage* ipl = cvCreateImage(cvSize(Route[fileIndex]->width,Route[fileIndex]->height),8,1);
		//cvCvtColor(Route[fileIndex],ipl,CV_RGB2GRAY);
		IplImage* ipl = Route[fileIndex];
		CvSize num_obs;
		CvSize roi = cvSize( ipl->roi ? ipl->roi->width : ipl->width,
			ipl->roi ? ipl->roi->height : ipl->height);
		CV_COUNT_OBS(&roi, &(params.dctSize), &(params.delta), &num_obs);//Macro: CV_COUNT_OBS来提取观察向量参数值
		obs_info_array[fileIndex] = cvCreateObsInfo(num_obs, vect_len);//Allocate a structure to each image
		CvImgObsInfo* info = obs_info_array[fileIndex];//to store in the array "info"
		//To get feature, DCT coefficients. 
		cvImgToObs_DCT(ipl, info->obs, params.dctSize, params.obsSize, params.delta);
		//uniform info to m_hmm
		cvUniformImgSegm(info, m_hmm);
		//cvReleaseImage(&ipl);
	}	


	cvInitMixSegm(obs_info_array, num_img, m_hmm);	// 由混合分量来分割HMM的每个内在状态的所有观测值

	bool trained = false;
	float old_likelihood = 0;//最相似参数值
	int counter = 0;

	while ((!trained) && (counter<params.maxiterations))
	{
		counter++;
		int j;
		// 计算所有HMM模型状态参数
		cvEstimateHMMStateParams(obs_info_array, 1, m_hmm);
		// 对植入的隐马尔可夫HMM模型，计算转移概率矩阵
		cvEstimateTransProb(obs_info_array, num_img, m_hmm);//为HMM状态的图像计算转移概率矩阵
		float likelihood = 0;
		for (j=0; j<num_img; j++)
		{
			cvEstimateObsProb(obs_info_array[j], m_hmm);//计算每一个可能的观察图像，计算每个可能发生高斯概率的内在HMM状态
			likelihood += cvEViterbi(obs_info_array[j], m_hmm);//执行嵌入式HMM的维特比算法,
			//Viterbi算法评估了给定图像之间的意见和给定的HMM的最佳匹配的可能性,并执行状态HMM的观测图像分割
		}
		likelihood /= num_img*obs_info_array[0]->obs_size;
		// 
		cvMixSegmL2(obs_info_array, num_img, m_hmm);
		trained = (fabs(likelihood - old_likelihood) < 0.01);//如果误差概率小于0.01，则将trained复制为1，否则为0
		old_likelihood = likelihood;
	}

	for(int i=0; i<num_img; i++)
	{
		cvReleaseObsInfo(&(obs_info_array[i]));//Release the structure.
	}
	m_trained = true;

}