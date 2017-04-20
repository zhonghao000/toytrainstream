#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <jpeglib.h>
#include <wiringPi.h>
#include "camera.h"
#include "graphics.h"

#define MAIN_TEXTURE_WIDTH 640
#define MAIN_TEXTURE_HEIGHT 480

unsigned char tmpbuff[MAIN_TEXTURE_WIDTH*MAIN_TEXTURE_HEIGHT*4];
const int Width = MAIN_TEXTURE_WIDTH;
const int Height = MAIN_TEXTURE_HEIGHT;

int process(unsigned char* tmpbuff, unsigned char* smoothGray)
{
  int i, j, k;
  int rowBase;
  int class1Coordinate[640];
  int coorMidean[480];
  int class1Num;
  int maximumLimit, minimumLimit;
  int *class1CoorPtr;
  int bendSum;
  int classNumber[480];
  int state = 0;
  //canny(smoothGray);
  
  //clock_t start = clock(), diff;
  //loadJpg("bend.jpg");
  //printf("width = %d, height = %d\n", Width, Height);
  

  for (i = 0; i < Width * Height; i ++)
    if ((0.2989 * (float)tmpbuff[i*4] + 0.5870 * (float)tmpbuff[i*4 + 1] + 0.1140 * (float)tmpbuff[i*4 + 2]) < 80)
      smoothGray[i] = 0;
    else
      smoothGray[i] = 200;
  //writeGrayJpg("step1.jpg", Width, Height, smoothGray);

  bendSum = 0;
  for (j = Height / 5; j < Height / 2; j++)
  {
    rowBase = (Height - 1 - j) * Width;
    class1Num = 0;
    class1CoorPtr = class1Coordinate;
    for (i = rowBase; i < rowBase + Width; i++) 
    {
      if (smoothGray[i] == 0)
      {
        *(class1CoorPtr++) = i - rowBase;
        class1Num += 1;
      }
    }
    maximumLimit = ((int) (0.5 * ((double) (class1Coordinate[class1Num * 3 / 4] - class1Coordinate[class1Num / 4])))) + class1Coordinate[class1Num * 3 / 4];
    minimumLimit = -((int) (0.5 * ((double) (class1Coordinate[class1Num * 3 / 4] - class1Coordinate[class1Num / 4])))) + class1Coordinate[class1Num / 4];
    if (maximumLimit > Width)
      maximumLimit = Width;
    if (minimumLimit < 0)
      minimumLimit = 0;
    classNumber[j] = 0;
    for (i = rowBase; i < rowBase + Width; i++) 
    {
      if (i - rowBase < minimumLimit || i - rowBase > maximumLimit)
      {
        smoothGray[i] = 200;
      }
      else if (smoothGray[i] == 0){
        classNumber[j] += 1;
      }
    }
    if (j > 5 + Height / 5)
    {
      if (classNumber[j] < classNumber[j-6] / 2)
      {
        state = 2;
        break;
      }
    }
    //printf("%d:%d\n", 480-j, classNumber[j]);
    coorMidean[j] = (maximumLimit + minimumLimit)/2;
    //printf("%d ", coorMidean[i]);
    if (j >= 4 + Height / 5)
    {
      for (k = 1; k < 5; k++)
        coorMidean[j] += coorMidean[j - k];
      coorMidean[j] /= 5;
      bendSum += coorMidean[j] - coorMidean[j-4];
      if (j >= 34 + Height / 5)
        bendSum -= coorMidean[j-30] - coorMidean[j-34];
      if (abs(bendSum) >= 50)
      {
        state = 1;
        if (abs(bendSum) >= 150)
          break;
      }
      //printf("%d:%d %d\n",480 - j, coorMidean[j] - coorMidean[j-4], bendSum);
    }
    
    //printf("%d ", class1Num);
  }

  printf("status = %d\n", state);

  /*
  char file[20] = "testChar";
  create_marks_csv(file,smoothGray);
  writeGrayJpg("step4.jpg", Width, Height, smoothGray);
    //printf("%d ", smoothGray[i]);
  */

  return state;
}

//entry point
int main(int argc, const char **argv)
{
        if (wiringPiSetup () == -1)
    		return 1 ;
 
 	pinMode (0, OUTPUT) ;         // aka BCM_GPIO pin 11
  	pinMode (2, OUTPUT) ; 	// aka BCM_GPIO pin 13

  	
	//should the camera convert frame data from yuv to argb automatically?
	bool do_argb_conversion = true;

	//how many detail levels (1 = just the capture res, > 1 goes down by half each level, 4 max)
	int num_levels = 1;
	int state = 0;

	//init graphics and the camera
	InitGraphics();
	CCamera* cam = StartCamera(MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT,30,num_levels,do_argb_conversion);

	//create 4 textures of decreasing size
	GfxTexture textures[4];
	for(int texidx = 0; texidx < num_levels; texidx++)
		textures[texidx].Create(MAIN_TEXTURE_WIDTH >> texidx, MAIN_TEXTURE_HEIGHT >> texidx);

	unsigned char *smoothGray = (unsigned char*) malloc(MAIN_TEXTURE_WIDTH * MAIN_TEXTURE_HEIGHT * sizeof(unsigned char));

	printf("Running frame loop\n");
	int texidx = 0;
	for(int i = 0; i < 300; i++)
	{
		//lock the chosen frame buffer, and copy it directly into the corresponding open gl texture
		const void* frame_data; int frame_sz;
		if(cam->BeginReadFrame(texidx,frame_data,frame_sz))
		{
			memcpy(tmpbuff,frame_data,frame_sz);
			textures[texidx].SetPixels(tmpbuff);
			//printf("%d %d %d %d %d %d %d %d\n", tmpbuff[0], tmpbuff[1], tmpbuff[2],tmpbuff[3],tmpbuff[4], tmpbuff[5], tmpbuff[6],tmpbuff[7]);
			//printf("%d ", i);
			state = process(tmpbuff, smoothGray);

			cam->EndReadFrame(texidx);
		}

		if (state == 0){
			digitalWrite (0, 1) ;       // normal speed
    			digitalWrite (2, 1) ;
		}
		else if (state == 1){
			digitalWrite (0, 0) ;       // slow
    			digitalWrite (2, 1) ;
		}
		else {
			digitalWrite (0, 0) ;       // stop
    			digitalWrite (2, 0) ;
		}

		//begin frame, draw the texture then end frame (the bit of maths just fits the image to the screen while maintaining aspect ratio)
		
		BeginFrame();
		float aspect_ratio = float(MAIN_TEXTURE_WIDTH)/float(MAIN_TEXTURE_HEIGHT);
		float screen_aspect_ratio = 1280.f/720.f;
		DrawTextureRect(&textures[texidx],-aspect_ratio/screen_aspect_ratio,-1.f,aspect_ratio/screen_aspect_ratio,1.f);
		EndFrame();
		
	}
	StopCamera();
	free(smoothGray);
}
