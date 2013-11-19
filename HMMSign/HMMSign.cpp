#include "stdafx.h"
#include<iostream>
#include<fstream>
#include<vector>
#include<algorithm>
#include<string>
#include <opencv2\opencv.hpp>
#include<atlstr.h>
#include<cmath>
#include <direct.h>
#include <cvaux.h>
#include "Sign.h"
using namespace std;
using namespace cv;

//#define ReadFromDat  //Read data from five saved gallery files. There must be the files of course.

#ifndef ReadFromDat
	#define OutPutImages
#endif

#define GalleryNum  5
#define SIZE 64
#define Posture_num 370         //Posture number
#define LRB 3                   //Left, right, both
#define FusedLRB 1
#define HOG_dimension 1764//720
#define MaxCombine 1000
#define MaxOri   1000
#define MarryThre 0.5

struct Pair
{
	int man;
	int woman;
	double love;
	int married;  //1: married. 0: unmarried. 2: may be
};

struct ForSaveImage
{
	int galleryIndex;
	int index;
};

CString Add="\\*.*";
ifstream infile;
IplImage* choose_pic[10000];    //To choose the best picture

vector<IplImage*> Route[GalleryNum][Posture_num][LRB];//Picture in file folder p50
vector<double> HOG[GalleryNum][Posture_num][LRB][MaxOri];//HOG feature for each key frame.
int  keyFrameNo[GalleryNum][Posture_num][LRB];  //Just LRB = 0 is used. 

ofstream outfile1;
ofstream outfile2;
ofstream outfile1_csv;
ofstream outfile_LRBLabel;

vector<double> HOG_final[Posture_num][LRB][MaxCombine];
int     keyFrameNo_final[Posture_num][LRB];
ForSaveImage myForSaveImage[Posture_num][LRB][MaxCombine];
int isCombined[Posture_num][LRB][MaxCombine]; //1: yes; 0: no.


double img_distance(IplImage *dst1,IplImage *dst2)//返回两个图像的欧几里得距离
{
	int i,j;
	uchar *ptr1;
	uchar *ptr2;

	double result=0.0;////////////
	for(i=0;i<dst1->height;i++)
	{
		ptr1=(uchar *)(dst1->imageData+i*dst1->widthStep);
		ptr2=(uchar *)(dst2->imageData+i*dst2->widthStep);

		for(j=0;j<dst1->width;j++)
			result+=(ptr1[j*dst1->nChannels]-ptr2[j*dst2->nChannels])*(ptr1[j*dst1->nChannels]-ptr2[j*dst2->nChannels]);
	}
	result=sqrt(result);
	return result;
}

IplImage *Resize(IplImage *_img)//Resize in OpenCV
{
	IplImage *_dst=cvCreateImage(cvSize(SIZE,SIZE),_img->depth,_img->nChannels);
	cvResize(_img,_dst);
	return _dst;
}

void TraverseAllRoute(CString BaseRoute,vector<IplImage *> Route[Posture_num][LRB])//存储要处理的文件（图片）的路径
{
	WIN32_FIND_DATA FileData;
	HANDLE handle=FindFirstFile(BaseRoute+Add,&FileData);

	if(handle==INVALID_HANDLE_VALUE)
	{
		//cout<<"访问文件失败!"<<endl;
		//exit(0);
		return ;
	}
	CString temp;
	int i,j,k;//
	int m,n;//i,j,k,m,n,mn均为循环变量
	int Sec;//
	int Lindex,Rindex;//分别表示手势序号及其对应的左手、右手和双手
	int Count_keyframe;//关键帧数
	int a,b;//每个关键帧中所包括的图片序列起始和结束的编号
	char s[10];//临时字符串数组，用于使用函数itoa
	IplImage *T_img;//
	IplImage *T_avg=cvCreateImage(cvSize(SIZE,SIZE),8,1);//存放均值图像
	uchar *pp;//
	uchar *qq;//
	int Sum[105][105];//存储图片灰度求和的结果

	int Image_size;/////////////////////////////////
	double Image_distance;/////////////////////////
	double Image_temp;///////////////
	int Image_index;////////////////////////////

	while( FindNextFile(handle,&FileData) )
	{
		temp=FileData.cFileName;
		if( strcmp( temp.Right(3),"txt" )==0 )//查找txt文件
		{
			Sec=0;
// 			for(i=0;i<BaseRoute.GetLength();i++)
// 			{
// 				if( BaseRoute[i]=='_' )//
// 				{
// 					Sec++;
// 					if(Sec==1)//获取是第几个手势(手势编号)
// 					{
// 						Lindex=(BaseRoute[i+1]-48)*1000 + (BaseRoute[i+2]-48)*100 + (BaseRoute[i+3]-48)*10 + (BaseRoute[i+4]-48);
// 						break;
// 					}
// 				}
// 			}
			for(i=0;i<BaseRoute.GetLength();i++)
			{
				if( BaseRoute[i]== '\\' ) //
				{
					Sec++;
				}
				else if (Sec == 1 && BaseRoute[i]!='P' )
				{
					Sec = 0;
				}
				else if (Sec == 1 && BaseRoute[i]=='P' )
				{
					Lindex=(BaseRoute[i+4]-48)*1000 + (BaseRoute[i+5]-48)*100 + (BaseRoute[i+6]-48)*10 + (BaseRoute[i+7]-48);
					break ;
				}
			}


			if( temp[0]=='l' )//左手LeftHand
				Rindex=0;
			else if( temp[0]=='r' )//右手RightHand
				Rindex=1;
			else 
				Rindex=2;//双手Both

			infile.open(BaseRoute+"\\"+temp,ios::in);//打开搜到的txt文档
			infile>>Count_keyframe;//输入关键帧数
			CString Temp;//

			for(i=0;i<Count_keyframe;i++)
			{
				Image_size=0;
				memset(Sum,0,sizeof(Sum));
				Image_distance=1.0*0xffffff;

				infile>>a>>b;
				for(j=a;j<=b;j++)
				{
						//Changed by Hanjie Wang.
// 					if( j>0 && j<10 )
// 						Temp=BaseRoute+"\\000"+itoa( j , s , 10 )+".jpg";
// 					else if( j>=10 && j<100 )
// 						Temp=BaseRoute+"\\00"+itoa( j , s , 10 )+".jpg";
// 					else if( j>=100 )
// 						Temp=BaseRoute+"\\0"+itoa( j , s , 10 )+".jpg";

					Temp=BaseRoute+"\\"+itoa( j , s , 10 )+".jpg";
					//IplImage *T_img;
					T_img=cvLoadImage(Temp,0);
					if (T_img == NULL)
					{
						T_img=cvLoadImage("black.jpg",0);
						T_img=Resize(T_img);
						Route[Lindex][Rindex].push_back(T_img);
					}
					else if(T_img!=NULL)//存在这幅图像
					{
						T_img=Resize(T_img);//在这里进行size的归一化
						cvSmooth(T_img,T_img,CV_GAUSSIAN,5,3);//平滑处理，消除噪声
						Route[Lindex][Rindex].push_back(T_img);
// 						for(m=0 ; m<T_img->height ; m++)
// 						{
// 							pp=(uchar *)(T_img->imageData+m*T_img->widthStep);
// 							for(n=0;n<T_img->width;n++)
// 							{
// 								Sum[m][n]+=pp[n*T_img->nChannels];
// 							}
// 						}
// 						choose_pic[Image_size++]=T_img;
					}
					//cvReleaseImage(&T_img);
				}

// 				if(Image_size==0)
// 				{
// 					//outfile_LRBLabel<<0<<'\t';
// 				}
// 				else
// 				{
// 					for(m=0;m<SIZE;m++)
// 					{
// 						qq=(uchar *)(T_avg->imageData+m*T_avg->widthStep);
// 						for(n=0;n<SIZE;n++)
// 						{
// 							Sum[m][n]=Sum[m][n]/Image_size;
// 							qq[n*T_avg->nChannels]=Sum[m][n];
// 						}
// 					}
// 					Image_index = 0;
// 					for(k=0;k<Image_size;k++)
// 					{
// 						bool black = true;
// 						for(m=0 ; m<choose_pic[k]->height ; m++)
// 						{ 
// 							bool inBreak =  false;
// 							pp=(uchar *)(choose_pic[k]->imageData+m*choose_pic[k]->widthStep);
// 							for(n=0;n<choose_pic[k]->width;n++)
// 							{
// 								if (pp[n]>0)
// 								{
// 									black = false;
// 									inBreak = true;
// 									break;
// 								}
// 							}
// 							if (inBreak)
// 							{
// 								break;
// 							}
// 						}
// 
// 						if (!black)
// 						{
// 							Image_temp=img_distance(choose_pic[k],T_avg);
// 							if(Image_temp<Image_distance)
// 							{
// 								Image_distance=Image_temp;
// 								Image_index=k;
// 							}
// 						}
// 
// 						
// 					}
// 					Route[Lindex][Rindex].push_back(choose_pic[Image_index]);
// 					//outfile_LRBLabel<<1<<'\t';
// 				}
			}
			infile.close();
		}
		if( strcmp(temp,"..") )
		{
			TraverseAllRoute(BaseRoute+"\\"+temp,Route);
		}
	}
	//outfile_LRBLabel<<endl;
}

bool GetHOGHistogram_Patch(IplImage *img,vector<double> &hog_hist)//取得图像img的HOG特征向量
{
	//HOGDescriptor *hog=new HOGDescriptor(cvSize(SIZE,SIZE),cvSize(8,8),cvSize(4,4),cvSize(4,4),9);
	HOGDescriptor *hog=new HOGDescriptor(cvSize(SIZE,SIZE),cvSize(16,16),cvSize(8,8),cvSize(8,8),9);
	//HOGDescriptor *hog=new HOGDescriptor(cvSize(SIZE,SIZE),cvSize(32,32),cvSize(16,16),cvSize(16,16),20);
	/////////////////////window大小为64*64，block大小为8*8，block步长为4*4，cell大小为4*4
	Mat handMat(img);

	vector<float> *descriptors = new std::vector<float>();

	hog->compute(handMat, *descriptors,Size(0,0), Size(0,0));

	////////////////////window步长为0*0
	double total=0;
	int i;
	for(i=0;i<descriptors->size();i++)
		total+=abs((*descriptors)[i]);
	//	total=sqrt(total);
	for(i=0;i<descriptors->size();i++)
		hog_hist.push_back((*descriptors)[i]/total);
	return true; 
}

	//Measuring two HOG features.
double Histogram(vector<double>vec1,vector<double>vec2)
{
	double mat_score=0.0;
	int i;
	int _Size=vec1.size();
	for(i=0;i<_Size;i++)
	{
		mat_score+=vec1[i]<vec2[i] ? vec1[i] : vec2[i];
	}
	return  mat_score;
}

/*************************************************
Function:       saveKeyFrame_combine
Description:    Save key frames. 
Calls:          None
Input:          1. posture: the index of posture. [0-369]
				2. gallery: the index of gallery. [0-5]
				3. lrb: left, right or both hand. [0-3]
				4. indexNow: the current index for output as in fused gallery.
				5. indexOri: the original index in separated gallery.
				6. myImage: the image used for saving.
Output:         1. Images saved in file folder "s_filefolder"
Return:         void
Others:         If "ReadFromDat" is not defined, this function will work.
*************************************************/
void saveKeyFrame_combine(int posture, int gallery, int lrb, int indexNow, int indexOri, IplImage* myImage)
{
#ifdef OutPutImages
	int i,j,k;
	CString s_filefolder;
	CString s_ImgFileName;
	s_filefolder.Format("..\\output\\keyFrame_combine_LRToOne");
	_mkdir(s_filefolder);
	s_filefolder.Format("..\\output\\keyFrame_combine_LRToOne\\%d",posture);
	_mkdir(s_filefolder);

	// 	gallery = 0;
	// 	indexOri = 0;

	if (lrb==0)
	{
		s_ImgFileName.Format("..\\output\\keyFrame_combine_LRToOne\\%d\\LRB_%d_G%d_%d.jpg",posture,indexNow,gallery,indexOri);
	}
	else if (lrb==1)
	{
		s_ImgFileName.Format("..\\output\\keyFrame_combine_LRToOne\\%d\\right_%d_G%d_%d.jpg",posture,indexNow,gallery,indexOri);
	}
	else if (lrb==2)
	{
		s_ImgFileName.Format("..\\output\\keyFrame_combine_LRToOne\\%d\\both_%d_G%d_%d.jpg",posture,indexNow,gallery,indexOri);
	}

// 	IplImage* wideImage = cvCreateImage(cvSize(SIZE*2,SIZE),8,1);
// 	cvResize(myImage, wideImage);

	cvSaveImage(s_ImgFileName, myImage);
#endif
}

void saveKeyFrame_separate( int filefolder,int wordIndex,int keyFrameIndex,  IplImage* Route)
{
	CString s_filefolder;
	CString s_ImgFileName;
	s_filefolder.Format("..\\output\\keyFrame_LRToOne");
	_mkdir(s_filefolder);//Create the folder.
	s_filefolder.Format("..\\output\\keyFrame_LRToOne\\P%d",filefolder);
	_mkdir(s_filefolder);//Create the folder.
	s_filefolder.Format("..\\output\\keyFrame_LRToOne\\P%d\\%d",filefolder,wordIndex);
	_mkdir(s_filefolder);//Create the folder.

	s_ImgFileName.Format("..\\output\\keyFrame_LRToOne\\P%d\\%d\\LRB_%d.jpg",filefolder,wordIndex,keyFrameIndex);
// 	IplImage* wideImage = cvCreateImage(cvSize(SIZE*2,SIZE),8,1);
// 	cvResize(Route, wideImage);
	cvSaveImage(s_ImgFileName, Route);

// 	for (i=0; i<posture_num; i++)
// 	{
// 		for(j=0;j<FusedLRB;j++)
// 		{
// 			for(k=0;k<Route[i][j].size();k++)
// 			{
// 				if (Route[i][j][k]!=NULL)
// 				{
// 					s_filefolder.Format("..\\output\\keyFrame_LRToOne\\P%d\\%d",dataIndex,i);
// 					_mkdir(s_filefolder);//Create the folder.
// 					if (j==0)
// 					{
// 						s_ImgFileName.Format("..\\output\\keyFrame_LRToOne\\P%d\\%d\\left_%d.jpg",dataIndex,i,k);
// 					}
// 					else if (j==1)
// 					{
// 						s_ImgFileName.Format("..\\output\\keyFrame_LRToOne\\P%d\\%d\\right_%d.jpg",dataIndex,i,k);
// 					}
// 					else if (j==2)
// 					{
// 						s_ImgFileName.Format("..\\output\\keyFrame_LRToOne\\P%d\\%d\\both_%d.jpg",dataIndex,i,k);
// 					}
// 
// 					cvSaveImage(s_ImgFileName, Route[i][j][k]);
// 				}
// 
// 			}
// 		}
// 	}

}

/*************************************************
Function:       dataTofeature
Description:    Read data and compute the HoG feature separately.
Calls:          1. TraverseAllRoute
				2. GetHOGHistogram_Patch
Input:          1. Testdata: folder name. 
				2. posture_num: the number of postures
				3. ikeyFrameNo[Reference]: array of int. The frame number of each gallery.
				4. Route_0[Reference]: array of Iplimage. The key frame images of each gallery.
				5. HOG_0[Reference]: array of vector<double>. The HOG feature each gallery. 
				6. folderIndex: the index of folder, P5X. {50,51,52,53,54}
Output:         1. ikeyFrameNo
				2. Route_0
				3. HOG_0
Return:         void
Others:         ikeyFrameNo, Route_0 and HOG_0 are defined global.
*************************************************/
void dataTofeature(	CString Testdata, 
					int posture_num,
					int ikeyFrameNo[][LRB],
					vector<IplImage*>Route_0[][LRB], 
					vector<double>HOG_0[][LRB][MaxOri], 
					int folderIndex)
{
	int i,j,k,m,n;
	CString folderName;
	//folderName.Format(folderIndex);
	folderName.Format("%d", folderIndex);
	int keyframe_count;
	//CString::Format(folderName,folderIndex);
	TraverseAllRoute("D:\\iData\\Kinect sign data\\Test\\"+Testdata+"\\P"+folderName,Route_0);
	cout<<"("<<folderIndex<<")Route searching of file p"<<folderIndex<< " is finished."<<endl;
	for(i=0;i<posture_num;i++)
	{
		keyframe_count = Route_0[i][0].size(); //Since all the three channel has the same size. 
		ikeyFrameNo[i][0] = keyframe_count;
		outfile2.write( (char *)&keyframe_count,sizeof(keyframe_count));
		//cout<<"P"<<folderIndex<<" sentence: "<<i<<" key frame number: "<<keyframe_count<<endl;
		
		for(k=0;k<keyframe_count;k++)
		{
			
			IplImage* LRImage = cvCreateImage(cvSize(SIZE*2,SIZE),8,1);
			for (m=0;m<SIZE;m++)
			{ 
				uchar* src_left = (uchar*)(Route_0[i][0][k]->imageData + m*Route_0[i][0][k]->widthStep);
				uchar* src_right = (uchar*)(Route_0[i][1][k]->imageData + m*Route_0[i][1][k]->widthStep);
				uchar* src_LR = (uchar*)(LRImage->imageData + m*LRImage->widthStep);
				for (n=0;n<SIZE;n++)
				{
					src_LR[n] = src_left[n];
				}
				for (n=SIZE;n<2*SIZE;n++)
				{
					src_LR[n] = src_right[n-SIZE];
				}

			}
			LRImage = Resize(LRImage);

			//saveKeyFrame_separate(folderIndex,i,k+100,Route_0[i][0][k]);
			//saveKeyFrame_separate(folderIndex,i,k+200,Route_0[i][1][k]);

			bool notall0 = false;
			for (m=0;m<SIZE;m++)
			{ 
				bool temp = false;
				//uchar* src_ptr = (uchar*)(LRImage->imageData + m*LRImage->widthStep);
				uchar* src_left = (uchar*)(Route_0[i][0][k]->imageData + m*Route_0[i][0][k]->widthStep);
				uchar* src_right = (uchar*)(Route_0[i][1][k]->imageData + m*Route_0[i][1][k]->widthStep);
				for (n=0;n<SIZE;n++)
				{
					if (/*src_ptr[n] > 0*/  src_left[n] >0 || src_right[n]>0)
					{
						notall0 = true;
						temp = true;
						break;
					}
				}
				if (temp)
				{
					break;
				}
			}

			if (!notall0)
			{
				//GetHOGHistogram_Patch(Route_0[i][2][k],HOG_0[i][0][k]);
				saveKeyFrame_separate(folderIndex,i,k,Route_0[i][2][k]);
				cvCopy(Route_0[i][2][k],Route_0[i][0][k]);  //They are at last stored in the Route_0[i][0][k]
			}
			else
			{
				//GetHOGHistogram_Patch(LRImage,HOG_0[i][0][k]);
				saveKeyFrame_separate(folderIndex,i,k,LRImage);
				cvCopy(LRImage,Route_0[i][0][k]);  //They are at last stored in the Route_0[i][0][k]
			}

			//outfile1.write( (char *)&HOG_0[i][0][k][0],HOG_dimension*sizeof(double) );
// 			for (m=0; m<HOG_dimension; m++)
// 			{
// 				outfile1_csv<<HOG_0[i][0][k][m]<<",";
// 			}
			
			cvReleaseImage(&LRImage);
		}
		//outfile1_csv<<endl;
		/*for(j=0;j<LRB;j++)
		{
			keyframe_count = Route_0[i][j].size();
			ikeyFrameNo[i][j] = keyframe_count;
			outfile2.write( (char *)&keyframe_count,sizeof(keyframe_count) );
			for(k=0;k<keyframe_count;k++)
			{
				bool notall0 = false;
				for (m=0;m<SIZE;m++)
				{ 
					bool temp = false;
					uchar* src_ptr = (uchar*)(Route_0[i][j][k]->imageData + m*Route_0[i][j][k]->widthStep);
					for (n=0;n<SIZE;n++)
					{
						if (src_ptr[n] >0)
						{
							notall0 = true;
							temp = true;
							break;
						}
					}
					if (temp)
					{
						break;
					}
				}

				if (!notall0)
				{
					for (m=0; m<HOG_dimension; m++)
					{
						HOG_0[i][j][k].push_back(0.0);
					}
				}
				else
				{
					GetHOGHistogram_Patch(Route_0[i][j][k],HOG_0[i][j][k]);
				}
				outfile1.write( (char *)&HOG_0[i][j][k][0],HOG_dimension*sizeof(double) );
			}
		}*/
	}
	cout<<"("<<folderIndex<<")HOG features in file p"<<folderIndex<< " have been computed."<<endl;

// #ifdef OutPutImages
// 	saveKeyFrame_separate(posture_num,folderIndex,Route_0);
// #endif
	
}
	
/*************************************************
Function:       ReadDataFromGallery
Description:    Read separated galleries from saved file. It is fast for debugging.
Calls:          None. 
Input:          1. route: the route of saved gallery file.
				2. Gallery_num: the number of gallery. It is 5 temporally.
				3. ikeyFrameNo[Reference]: array of int. The frame number of all the galleries.
				4. HOG_X[Reference]: array of vector<double>. The HOG feature of all the galleries. 
Output:         1. ikeyFrameNo
				2. HOG_X
Return:         void
Others:         There must be saved files of separated galleries.
				The ikeyFrameNo and HOG_X are defined global.
*************************************************/
void ReadDataFromGallery(	CString route, 
							int Gallery_num, 
							int ikeyFrameNo[][Posture_num][LRB],
							vector<double>HOG_0[][LRB][MaxOri],
							vector<double>HOG_1[][LRB][MaxOri],
							vector<double>HOG_2[][LRB][MaxOri],
							vector<double>HOG_3[][LRB][MaxOri],
							vector<double>HOG_4[][LRB][MaxOri])
{
	int i, j, k, galleryIndex, m;
	int* Label_sequence;     //Original label sequence.
	int* Label_sequence_locate;
	double* p_gallery;           //Gallery HOG

	int labelSize = Gallery_num*Posture_num*FusedLRB;  //Label_size=5*370*3
	Label_sequence = new int[labelSize];
	Label_sequence_locate = new int[labelSize];

	ifstream infile1;
	infile1.open(route+"\\Gallery_Label.dat",ios::binary);
	infile1.read( (char *)Label_sequence, labelSize*sizeof(int) );//将Gallery_Label中的数据读到数组Label_sequence1中
	infile1.close();

	int keyFrameIntotal = 0;
	*(Label_sequence_locate+0) = *(Label_sequence + 0);
	for(i=0;i<labelSize;i++)
	{
		keyFrameIntotal += *(Label_sequence + i);
		if (i>0)
		{
			*(Label_sequence_locate+i) = *(Label_sequence_locate+i-1) + *(Label_sequence + i);
		}
	}
	cout<<"Label has been read into memory"<<endl;
	int HOGsize=keyFrameIntotal * HOG_dimension;//HOG_size
	p_gallery=new double[HOGsize];                         //p_gallery

	ifstream infile2;
	infile2.open(route+"\\Gallery_Data.dat",ios::binary);
	infile2.read((char*)p_gallery,HOGsize * sizeof(double));
	infile2.close();
	cout<<"Gallery has been read into the memory"<<endl;

	//////////////////////////////////////////////////////////////////////////
	// 	int testFlag[375];
	// 	for (i=0; i<Posture_num; i++)
	// 	{
	// 		infileMask>>testFlag[i];
	// 	}
	int count;
	for (galleryIndex = 0; galleryIndex<Gallery_num; galleryIndex++)
	{
		count = 0;
		for(i=0; i<Posture_num; i++)                             //Posture
		{
			//if (testFlag[i] == 1)
			{
				for(j=0;j<FusedLRB;j++)                                         //Left, right, both
				{
					ikeyFrameNo[galleryIndex][count][j] = *(Label_sequence + galleryIndex*FusedLRB*Posture_num + i*FusedLRB + j);
					int frameLocation;
					if (galleryIndex == 0 && i == 0 && j == 0)
					{
						frameLocation = 0;
					}
					else
					{
						frameLocation = *(Label_sequence_locate + galleryIndex*FusedLRB*Posture_num + i*FusedLRB + j-1);
					}

					for(k=0; k<ikeyFrameNo[galleryIndex][count][j]; k++)            //Key frame
					{
						for (m=0; m<HOG_dimension; m++)
						{
							HOG[galleryIndex][count][j][k].push_back(*(p_gallery + HOG_dimension*(frameLocation+k) + m));
						}
					}
				}
				count +=1;
			}
		}
		cout<<"Gallery "<<galleryIndex<<" has been read into array"<<endl;
	}



	delete[] p_gallery;
	delete[] Label_sequence;
}


/*************************************************
Function:       GalleryCombine
Description:    Fuse the galleries into one final gallery.
Calls:          1. Histogram.
				2. saveKeyFrame_combine.
Input:          1. HOG_X:     Array of vector. Data of five separated galleries.
				2. keyFrameNo_X: Array of int. Label of five separated galleries.
Output:         1. HOG_final: Array of vector. Data of fused gallery.
				2. keyFrameNo_final:Array of int. Label of fused gallery.
				3. isCombined: array of int. Indicator of combined gesture.
Return:         void
Others:         3 variants in output are all global.
*************************************************/
void GalleryCombine(vector<double> HOG0[][LRB][MaxOri],
					vector<double> HOG1[][LRB][MaxOri],
					vector<double> HOG2[][LRB][MaxOri],
					vector<double> HOG3[][LRB][MaxOri],
					vector<double> HOG4[][LRB][MaxOri],
					int keyFrameNo0[][LRB],
					int keyFrameNo1[][LRB],
					int keyFrameNo2[][LRB],
					int keyFrameNo3[][LRB],
					int keyFrameNo4[][LRB])
{
	int i,j,k,g,m,n;
	int  keyFrameNo[GalleryNum][Posture_num][LRB];

		//Initial isCombined.
	for(i=0;i<Posture_num;i++)
	{
		for(j=0;j<FusedLRB;j++)
		{
			for(k=0;k<MaxCombine;k++)
			{
				isCombined[i][j][k] = false;
			}
		}
	}

	for (i=0; i<GalleryNum; i++)
	{
		for (j=0; j<Posture_num; j++)
		{
			for (k=0; k<FusedLRB; k++)
			{
				if (i==0) keyFrameNo[0][j][k] = keyFrameNo0[j][k];
				if (i==1) keyFrameNo[1][j][k] = keyFrameNo1[j][k];
				if (i==2) keyFrameNo[2][j][k] = keyFrameNo2[j][k];
				if (i==3) keyFrameNo[3][j][k] = keyFrameNo3[j][k];
				if (i==4) keyFrameNo[4][j][k] = keyFrameNo4[j][k];
			}
		}
	}
	
	//Initial of x_final. Input the HOG_0 into it.
	for (i=0; i<Posture_num; i++)
	{
		for (j=0; j<FusedLRB; j++)
		{
			int sizeTemp = keyFrameNo[0][i][j];
			keyFrameNo_final[i][j] = sizeTemp;
			for (k=0; k<sizeTemp; k++)
			{
				HOG_final[i][j][k] = HOG0[i][j][k];
			}
		}
	}
	//Gallery combination
	for (i=0; i<Posture_num; i++)
	{
		cout<<"Posture"<<i<<endl;
		if (i == 9)
		{
			int a=0;
		}
		for (j=0; j<FusedLRB; j++)
		{
			for (g=1; g<GalleryNum; g++)
			{
				int finalSize = keyFrameNo_final[i][j];
				int currentSize = keyFrameNo[g][i][j];
				int pairNum = finalSize*currentSize; //The total number of key frame pairs
				vector<Pair> myPair;
				for (m=0; m<finalSize; m++)
				{
					for (n=0; n<currentSize; n++)
					{
						Pair tempPair;
						tempPair.man = m;
						tempPair.woman = n;
						if (g==1)
						{
							tempPair.love = Histogram(HOG_final[i][j][m],HOG1[i][j][n]);
						}
						else if (g==2)
						{
							tempPair.love = Histogram(HOG_final[i][j][m],HOG2[i][j][n]);
						}
						else if (g==3)
						{
							tempPair.love = Histogram(HOG_final[i][j][m],HOG3[i][j][n]);
						}
						else if (g==4)
						{
							tempPair.love = Histogram(HOG_final[i][j][m],HOG4[i][j][n]);
						}
						tempPair.married = 2;
						myPair.push_back(tempPair);
					}
				}
				//////////////////////////////////////////////////////////////////////////
				//Find the hog_final from myPair.
				int Maybe_num = 0;
				for (k=0; k<pairNum; k++)
				{
					if (myPair[k].married == 2)
					{
						Maybe_num++;
					}
				}
				//Label the married.
				//int count = 0;
				while (Maybe_num > 0 /*&& count<pairNum*/)
				{
					//count++;
					//Find the largest love and marry them.
					double max = 0.0;
					int maxIndex = 0;
					for (k=0; k<pairNum; k++)
					{
						if (myPair[k].married==2 && myPair[k].love >= max)
						{
							max = myPair[k].love;
							maxIndex = k;
						}
					}
					if (myPair[maxIndex].love > MarryThre)
					{
						myPair[maxIndex].married = 1;

						//Unmarried the related others. 
						for (k=0; k<pairNum; k++)
						{
							if (k!=maxIndex)
							{
								bool sad = false;   //If sad is true, they will be unmarried (0).
								if (myPair[k].man == myPair[maxIndex].man)
								{
									sad = true;
								}
								if (myPair[k].woman == myPair[maxIndex].woman)
								{
									sad = true;
								}
								if (myPair[k].man > myPair[maxIndex].man && myPair[k].woman < myPair[maxIndex].woman)
								{
									sad = true;
								}
								if (myPair[k].man < myPair[maxIndex].man && myPair[k].woman > myPair[maxIndex].woman)
								{
									sad = true;
								}
								if (sad)
								{
									myPair[k].married = 0;  //They can not be married.
								}
							}
						}

						Maybe_num = 0;
						for (k=0; k<pairNum; k++)
						{
							if (myPair[k].married == 2)
							{
								Maybe_num++;
							}
						}
					}
					else
					{
						for (k=0; k<pairNum; k++)
						{
							if (myPair[k].married == 2)
							{
								myPair[k].married = 0;
							}
						}

						break;
					}
				}

				//Combine the gallery.
				vector<double> HOGTemp[MaxCombine];
				int keyFrameNoTemp = keyFrameNo_final[i][j];
				ForSaveImage forSaveTemp[MaxCombine];
				//bool isCombineTemp[MaxCombine];

				for (k=0; k<MaxCombine; k++)
				{
					HOGTemp[k] = HOG_final[i][j][k];
					//isCombineTemp[k] = isCombined[i][j][k];
					HOG_final[i][j][k].clear();
					isCombined[i][j][k] = 0;
					forSaveTemp[k].galleryIndex = myForSaveImage[i][j][k].galleryIndex;
					forSaveTemp[k].index = myForSaveImage[i][j][k].index;
					myForSaveImage[i][j][k].galleryIndex = -1;
					myForSaveImage[i][j][k].index = -1;
				}

				int NoCount = 0;
				for (k=0; k<pairNum; k++)
				{
					if (myPair[k].married == 1)
					{
						NoCount++;
					}
				}
				NoCount = finalSize + currentSize - NoCount;
				keyFrameNo_final[i][j] = NoCount;
				NoCount = 0;

				//int manPoint = 0;
				int womanPoint = 0;

				for (k=0; k<keyFrameNoTemp; k++)
				{
					bool isMarry = false;
					int wife = 0;
					for (m=0; m<pairNum; m++)
					{
						if (myPair[m].man == k && myPair[m].married == 1)
						{
							isMarry = true;
							wife = myPair[m].woman;
						}

					}
					if (!isMarry)
					{
						HOG_final[i][j][NoCount] = HOGTemp[k];
						if (g==1)
						{
							myForSaveImage[i][j][NoCount].galleryIndex = 0;
							myForSaveImage[i][j][NoCount].index = k;
						}
						else
						{
							myForSaveImage[i][j][NoCount].galleryIndex = forSaveTemp[k].galleryIndex;
							myForSaveImage[i][j][NoCount].index = forSaveTemp[k].index;
						}
						
						NoCount++;
					}
					else if (isMarry)
					{
						for (n=womanPoint; n<wife; n++)
						{
							if (g==1)
							{
								HOG_final[i][j][NoCount] = HOG1[i][j][n];
								myForSaveImage[i][j][NoCount].galleryIndex = 1;
								myForSaveImage[i][j][NoCount].index = n;
							}
							else if (g==2)
							{
								HOG_final[i][j][NoCount] = HOG2[i][j][n];
								myForSaveImage[i][j][NoCount].galleryIndex = 2;
								myForSaveImage[i][j][NoCount].index = n;
							}
							else if (g==3)
							{
								HOG_final[i][j][NoCount] = HOG3[i][j][n];
								myForSaveImage[i][j][NoCount].galleryIndex = 3;
								myForSaveImage[i][j][NoCount].index = n;
							}
							else if (g==4)
							{
								HOG_final[i][j][NoCount] = HOG4[i][j][n];
								myForSaveImage[i][j][NoCount].galleryIndex = 4;
								myForSaveImage[i][j][NoCount].index = n;
							}
							NoCount++;
						}
						womanPoint = wife + 1;
						//////////////////////////////////////////////////////////////////////////
						//Choose one 
							//without weight
						//HOG_final[i][j][NoCount] = HOGTemp[k];  

							//with weight
						double weight = 0.5;
						for (n=0; n<HOG_dimension; n++)
						{
							double hog = 0.0;
							double hogtemp = 0.0;

							if (g==1) 
							{
								hog = HOG1[i][j][wife][n]*weight;
								hogtemp = HOGTemp[k][n]*(1-weight) + hog;
							}
							else if (g==2) 
							{
								hog = HOG2[i][j][wife][n]*weight;
								hogtemp = HOGTemp[k][n]*(1-weight) + hog;
							}
							else if (g==3) 
							{
								hog = HOG3[i][j][wife][n]*weight;
								hogtemp = HOGTemp[k][n]*(1-weight) + hog;
							}
							else if (g==4) 
							{
								hog = HOG4[i][j][wife][n]*weight;
								hogtemp = HOGTemp[k][n]*(1-weight) + hog;
							}
							HOG_final[i][j][NoCount].push_back(hogtemp);
						}

						isCombined[i][j][NoCount] = 1;
						
						
						//////////////////////////////////////////////////////////////////////////
						if (g==1)
						{
							myForSaveImage[i][j][NoCount].galleryIndex = 0;
							myForSaveImage[i][j][NoCount].index = k;
						}
						else
						{
							myForSaveImage[i][j][NoCount].galleryIndex = forSaveTemp[k].galleryIndex;
							myForSaveImage[i][j][NoCount].index = forSaveTemp[k].index;
						}
						NoCount++;
					}
				}

				for (k=womanPoint; k<currentSize; k++)
				{
					if (g==1)
					{
						HOG_final[i][j][NoCount] = HOG1[i][j][k];
						myForSaveImage[i][j][NoCount].galleryIndex = 1;
						myForSaveImage[i][j][NoCount].index = k;
					}
					else if (g==2)
					{
						HOG_final[i][j][NoCount] = HOG2[i][j][k];
						myForSaveImage[i][j][NoCount].galleryIndex = 2;
						myForSaveImage[i][j][NoCount].index = k;
					}
					else if (g==3)
					{
						HOG_final[i][j][NoCount] = HOG3[i][j][k];
						myForSaveImage[i][j][NoCount].galleryIndex = 3;
						myForSaveImage[i][j][NoCount].index = k;
					}
					else if (g==4)
					{
						HOG_final[i][j][NoCount] = HOG4[i][j][k];
						myForSaveImage[i][j][NoCount].galleryIndex = 4;
						myForSaveImage[i][j][NoCount].index = k;
					}
					NoCount++;

				}
				
			}
		}
	}


	//For outputting images
	int keyFrameTemp;
	for(i=0;i<Posture_num;i++)
	{
		for(j=0;j<FusedLRB;j++)
		{
			keyFrameTemp = keyFrameNo_final[i][j];
			for(k=0;k<keyFrameTemp;k++)
			{
				//int posture, int gallery, int lrb, int indexNow, int indexOri, IplImage* myImage
				int gallery = myForSaveImage[i][j][k].galleryIndex;
				int indexOri = myForSaveImage[i][j][k].index;
				saveKeyFrame_combine(i,gallery,j,k,indexOri,Route[gallery][i][j][indexOri]);
			}
		}
	}

}


int Recognize(IplImage *ipl, HMMParams m_hmmp, CSign m_sign[])
{
	float like_array[1000];
	CvSize cvroi = cvSize(ipl->roi ? ipl->roi->width : ipl->width,
                               ipl->roi ? ipl->roi->height : ipl->height);
    CvSize num_obs;
    CvImgObsInfo* info;
    
    CV_COUNT_OBS(&cvroi, &m_hmmp.dctSize, &m_hmmp.delta, &num_obs); 

    int vect_len = m_hmmp.obsSize.height * m_hmmp.obsSize.width;

	info = cvCreateObsInfo(num_obs, vect_len);
	
	cvImgToObs_DCT(ipl, info->obs, m_hmmp.dctSize, m_hmmp.obsSize, m_hmmp.delta);//Feature extraction

	float max_like = -100000000; 

	//CvEHMM* hmm = 0;
	for(int i=0;i<Posture_num;i++)
	{
		if (m_sign[i].m_trained == true)
		{
			CvEHMM* hmm = 0;
			hmm=m_sign[i].m_hmm;
			cvEstimateObsProb(info, hmm);         //Observation likelihood. 
			like_array[i] = cvEViterbi(info, hmm);//Viterbi algorithm.
		}
		else
		{
			like_array[i] = max_like;
		}
	}
    cvReleaseObsInfo(&info);

		//Search the best matched one. 
	float maxl = -FLT_MAX;
	int maxind = -1;

	for(int j = 0; j < Posture_num; j++)
	{
		if (like_array[j] > maxl)
		{
			maxl = like_array[j];
			maxind = j;
		}
	}

	return maxind;
}

int main()
{
	int i,j,k,g,m,n;
	CString TestdataFolder;
	TestdataFolder = "20130925";
	cout<<"**********Use data in file folder "<<TestdataFolder<<"**********"<<endl;

#ifndef ReadFromDat
	outfile1.open("..\\output\\Gallery_Data.dat",ios::binary | ios::out);
	outfile2.open("..\\output\\Gallery_Label.dat",ios::binary | ios::out);
	outfile1_csv.open("..\\output\\Gallery_Data.csv",ios::out);
	outfile_LRBLabel.open("..\\output\\LRBLabel.txt",ios::out);

	for (i=0; i<2; i++)
	{
		dataTofeature(TestdataFolder,Posture_num,keyFrameNo[i],Route[i],HOG[i],50+i);
		//TestdataFolder and 50+i are used for creating route. 
	}

	HMMParams m_hmmp;
	m_hmmp.stateNum[0] = 1;
	m_hmmp.stateNum[1] = 3;
	//m_hmmp.stateNum[2] = 6;
	//m_hmmp.stateNum[3] = 6;
	//m_hmmp.stateNum[4] = 6;
	//m_hmmp.stateNum[5] = 3;
	//m_hmmp.stateNum[6] = 3;
	//m_hmmp.stateNum[7] = 3;
	for( int i = 0; i < 128; i++ )
	{
		m_hmmp.mixNum[i] = 3;
	}
	m_hmmp.maxiterations = 80;
	m_hmmp.dctSize = cvSize(12, 12);
	m_hmmp.obsSize = cvSize(3, 3);
	m_hmmp.delta = cvSize(4, 4);

	CSign m_sign[Posture_num];

	for (int i=0; i<Posture_num; i++)
	{
		if (keyFrameNo[0][i][0]>0)
		{
			cout<<i<<" ";
			m_sign[i].TrainHAMM(m_hmmp,Route[0][i][0]);//train each m_hmm
		}
		
	}

	for (int w=0; w<Posture_num; w++)
	{
		cout<<"Sign ID ---------------"<<w<<"---------------"<<endl;
		for (int i=0; i<keyFrameNo[0][w][0]; i++)
		{
			int clasd = Recognize(Route[0][w][0][i], m_hmmp, m_sign);
			cout<<"The result is: "<<clasd<<endl;
		}
	}
	
	

	outfile1.close();
	outfile2.close();
	outfile1_csv.close();
	outfile_LRBLabel.close();

	//The combine step. The parameters can not be changed.
	GalleryCombine(HOG[0],HOG[1],HOG[2],HOG[3],HOG[4],
		keyFrameNo[0],keyFrameNo[1],keyFrameNo[2],keyFrameNo[3],keyFrameNo[4]);
#endif

#ifdef ReadFromDat
		//For gallery combination. Fast reading.
	CString routeGallery;
	routeGallery="D:\\iCode\\SignLanKinectCode\\galleryAndLabel";
	ReadDataFromGallery(routeGallery, GalleryNum, keyFrameNo,HOG_0,HOG_1,HOG_2,HOG_3,HOG_4);

	//The combine step. The parameters can be changed.
	//forP50: 1,1,2,3,4
	//forP51: 0,2,2,3,4
	//forP52: 0,1,3,3,4
	//forP53: 0,1,2,4,4
	//forP54: 0,1,2,3,3
	GalleryCombine(HOG_0,HOG_1,HOG_2,HOG_3,HOG_4,
		keyFrameNo[0],keyFrameNo[1],keyFrameNo[2],keyFrameNo[3],keyFrameNo[4]);
#endif
		
	ofstream outfileDataCombine;
	ofstream outfileLabelCombine;
	ofstream outfileFlagCombine;
	outfileDataCombine.open("..\\output\\Gallery_Data_Combine.dat",ios::binary | ios::out);
	outfileLabelCombine.open("..\\output\\Gallery_Label_Combine.dat",ios::binary | ios::out);
	outfileFlagCombine.open("..\\output\\Gallery_Flag_Combine.dat",ios::binary | ios::out);
	int keyFrameTemp;
	for(i=0;i<Posture_num;i++)
	{
		for(j=0;j<FusedLRB;j++)
		{
			keyFrameTemp = keyFrameNo_final[i][j];
			outfileLabelCombine.write((char*)&keyFrameTemp,sizeof(keyFrameTemp));
			for(k=0;k<keyFrameTemp;k++)
			{
				outfileDataCombine.write((char*)&HOG_final[i][j][k][0],HOG_dimension*sizeof(double));
				outfileFlagCombine.write((char*)&isCombined[i][j][k],sizeof(int));
			}
		}
	}
	outfileDataCombine.close();
	outfileLabelCombine.close();
	outfileFlagCombine.close();
	
	cout<<"Done"<<endl;
	getchar();
	return 0;
}
