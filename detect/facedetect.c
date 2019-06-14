#include "cv.h"
#include "highgui.h"
#include "cxcore.h"
#include "mmsystem.h"//导入声音头文件
#pragma comment(lib,"winmm.lib")//导入声音头文件库


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#ifdef _EiC
#define WIN32
#endif

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;

void detect_and_draw( IplImage* image );
double AreaPercent( IplImage* areadst );

const char* cascade_name =
    "haarcascade_frontalface_alt.xml";
/*    "haarcascade_profileface.xml";*/

double countsum;/*全局变量*/
double average;/*全局变量*/
int firstcount;/*全局变量*/
int secondcount;/*全局变量*/
int numberdetect;/*全局变量*/

int main( int argc, char** argv )
{
    CvCapture* capture = 0;
    IplImage *frame, *frame_copy = 0;
    int optlen = strlen("--cascade=");
    const char* input_name;


    if( argc > 1 && strncmp( argv[1], "--cascade=", optlen ) == 0 )
    {
        cascade_name = argv[1] + optlen;
        input_name = argc > 2 ? argv[2] : 0;
    }
    else
    {
        cascade_name = "data/haarcascades/haarcascade_frontalface_alt2.xml";
        input_name = argc > 1 ? argv[1] : 0;
    }

    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    
    if( !cascade )
    {
        fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
        fprintf( stderr,
        "Usage: facedetect --cascade=\"<cascade_path>\" [filename|camera_index]\n" );
        return -1;
    }
    storage = cvCreateMemStorage(0);
    
    if( !input_name || (isdigit(input_name[0]) && input_name[1] == '\0') )
        capture = cvCaptureFromCAM( !input_name ? 0 : input_name[0] - '0' );
    else
        capture = cvCaptureFromAVI( input_name ); 

    cvNamedWindow( "result", 1 );
	cvNamedWindow( "dst", 1 );

	if( capture )
    {
		extern int firstcount;
		extern int secondcount;
		extern int numberdetect;
		extern double countsum;
		extern double average;

		for(firstcount=0;firstcount<100;)  //取前100关键帧作为样本
        {
            if( !cvGrabFrame( capture ))
                break;
            frame = cvRetrieveFrame( capture );
            if( !frame )
                break;
            if( !frame_copy )
                frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
                                            IPL_DEPTH_8U, frame->nChannels );
            if( frame->origin == IPL_ORIGIN_TL )
                cvCopy( frame, frame_copy, 0 );
            else
                cvFlip( frame, frame_copy, 0 );
            
            detect_and_draw( frame_copy );

            if( cvWaitKey( 10 ) >= 0 )
                break;
        }

        average= countsum*0.6/100;  //取样本平均值，与比例系数相乘作为后续参照值
		printf("\naverage=%f\n",average);
		
		for(;;)
        {
            for( secondcount=0; secondcount<30; )
			{
				if( !cvGrabFrame( capture ))
                    break;
                    frame = cvRetrieveFrame( capture );
                if( !frame )
                    break;
                if( !frame_copy )
                    frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
                                                IPL_DEPTH_8U, frame->nChannels );
                if( frame->origin == IPL_ORIGIN_TL )
                    cvCopy( frame, frame_copy, 0 );
                else
                    cvFlip( frame, frame_copy, 0 );
            
                detect_and_draw( frame_copy );

                if( cvWaitKey( 10 ) >= 0 )
                    break;
			}
			if(numberdetect>6)
			{
				printf("对不起，你已经疲倦了，为了保证安全请及时刹车！！！");
				PlaySound("ALARM.wav", NULL, SND_FILENAME | SND_ASYNC);
			}
			    
			numberdetect=0;
        }

        cvReleaseImage( &frame_copy );
        cvReleaseCapture( &capture );
    }
    else
    {
        const char* filename = input_name ? input_name : (char*)"large_fUNc_21965e206097.jpg";
        IplImage* image = cvLoadImage( filename, 1 );

        if( image )
        {
            detect_and_draw( image );
            cvWaitKey(0);
            cvReleaseImage( &image );
        }
        else
        {
            /* assume it is a text file containing the
               list of the image filenames to be processed - one per line */
            FILE* f = fopen( filename, "rt" );
            if( f )
            {
                char buf[1000+1];
                while( fgets( buf, 1000, f ) )
                {
                    int len = (int)strlen(buf);
                    while( len > 0 && isspace(buf[len-1]) )
                        len--;
                    buf[len] = '\0';
                    image = cvLoadImage( buf, 1 );
                    if( image )
                    {
                        detect_and_draw( image );
                        cvWaitKey(0);
                        cvReleaseImage( &image );
                    }
                }
                fclose(f);
            }
        }

    }
    
    cvDestroyWindow("result");
	cvDestroyWindow("dst");

    return 0;
}

void detect_and_draw( IplImage* img )
{
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    double scale = 1.3;
    IplImage* dsb = cvCloneImage( img );
	IplImage* dst = cvCloneImage( img );
    IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
    IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
                         cvRound (img->height/scale)),
                     8, 1 );
    int i;

    cvCvtColor( img, gray, CV_BGR2GRAY );
    cvResize( gray, small_img, CV_INTER_LINEAR );
    cvEqualizeHist( small_img, small_img );
    cvClearMemStorage( storage );

    if( cascade )
    {   
		double length;
		extern int firstcount;
		extern int secondcount;
		extern double countsum;
		extern double average;
        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
                                            1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(30, 30) );
        t = (double)cvGetTickCount() - t;
        printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
			CvPoint left,right;
			left.x = cvRound((r->x + r->width/6)*scale);
            left.y = cvRound((r->y + r->height/4)*scale);
			right.x = cvRound((r->x + r->width*5/6)*scale);
            right.y = cvRound((r->y + r->height/2)*scale);
            cvRectangle( img, left, right, colors[i%8], 3, 8, 0 );
     		cvSetImageROI( dsb, cvRect( left.x, left.y, right.x-left.x, right.y-left.y ));
			dst = cvCreateImage( cvSize( right.x-left.x, right.y-left.y ), 8, 1 );
			cvCvtColor( dsb, dst, CV_BGR2GRAY);
			cvThreshold( dst, dst, 50, 255, CV_THRESH_BINARY);
			/*cvAdaptiveThreshold( dst, dst, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 21, 5 );*/
			length=AreaPercent(dst);
			printf("AreaPercent=%f\n",length);
			if(firstcount<100) 
			{
				firstcount++;
				printf("firstcountnumber=%d\n",firstcount);
				countsum+=length;
			}
			else
            {
				secondcount++;
                printf("secondcountnumber=%d\n",secondcount);
                if(length < average) 
					numberdetect++;  
			}

        }
    }
    cvShowImage( "dst", dst );
    cvShowImage( "result", img );
    cvReleaseImage( &dsb );
	cvReleaseImage( &dst );
	cvReleaseImage( &gray );
    cvReleaseImage( &small_img );
}

double AreaPercent( IplImage* areadst )//计算图像面积，黑色像素的百分比
{
	double areapercent,area;
	area=(areadst->width)*(areadst->height);
	areapercent=(double)(cvCountNonZero(areadst)/area);
	return 1-areapercent;
}     