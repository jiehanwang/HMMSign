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

	m_hmm = cvCreate2DHMM(params.stateNum, params.mixNum, vect_len); //Create a Markov model. state number, ���Ԫ����

	int num_img = Route.size();//���ļ��е�ͼƬ������ֵnum_imgs
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
		CV_COUNT_OBS(&roi, &(params.dctSize), &(params.delta), &num_obs);//Macro: CV_COUNT_OBS����ȡ�۲���������ֵ
		obs_info_array[fileIndex] = cvCreateObsInfo(num_obs, vect_len);//Allocate a structure to each image
		CvImgObsInfo* info = obs_info_array[fileIndex];//to store in the array "info"
		//To get feature, DCT coefficients. 
		cvImgToObs_DCT(ipl, info->obs, params.dctSize, params.obsSize, params.delta);
		//uniform info to m_hmm
		cvUniformImgSegm(info, m_hmm);
		//cvReleaseImage(&ipl);
	}	


	cvInitMixSegm(obs_info_array, num_img, m_hmm);	// �ɻ�Ϸ������ָ�HMM��ÿ������״̬�����й۲�ֵ

	bool trained = false;
	float old_likelihood = 0;//�����Ʋ���ֵ
	int counter = 0;

	while ((!trained) && (counter<params.maxiterations))
	{
		counter++;
		int j;
		// ��������HMMģ��״̬����
		cvEstimateHMMStateParams(obs_info_array, 1, m_hmm);
		// ��ֲ���������ɷ�HMMģ�ͣ�����ת�Ƹ��ʾ���
		cvEstimateTransProb(obs_info_array, num_img, m_hmm);//ΪHMM״̬��ͼ�����ת�Ƹ��ʾ���
		float likelihood = 0;
		for (j=0; j<num_img; j++)
		{
			cvEstimateObsProb(obs_info_array[j], m_hmm);//����ÿһ�����ܵĹ۲�ͼ�񣬼���ÿ�����ܷ�����˹���ʵ�����HMM״̬
			likelihood += cvEViterbi(obs_info_array[j], m_hmm);//ִ��Ƕ��ʽHMM��ά�ر��㷨,
			//Viterbi�㷨�����˸���ͼ��֮�������͸�����HMM�����ƥ��Ŀ�����,��ִ��״̬HMM�Ĺ۲�ͼ��ָ�
		}
		likelihood /= num_img*obs_info_array[0]->obs_size;
		// 
		cvMixSegmL2(obs_info_array, num_img, m_hmm);
		trained = (fabs(likelihood - old_likelihood) < 0.01);//���������С��0.01����trained����Ϊ1������Ϊ0
		old_likelihood = likelihood;
	}

	for(int i=0; i<num_img; i++)
	{
		cvReleaseObsInfo(&(obs_info_array[i]));//Release the structure.
	}
	m_trained = true;

}