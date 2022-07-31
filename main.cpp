// g++ main.cpp -ltbb -o binarka

#include <iostream>
#include <cmath>

#include <tbb/tbb.h>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

using namespace std;

const int size = 500;
char pixels[size][size][3];

unsigned char colors[][3] = {{191, 228, 118}, {117, 137, 191}, {249, 140, 182}, {200, 10, 30}, {154, 206, 233}, {255, 255, 255}};

class Lapunov
{
public:
	char Sequence[16];
	const int noColors = 255;
	float MinR, MaxR, MinX, MaxX;
	char NoSeq = 16;
	float LapMin = -2;
	int sample;

	int DecToBin(int Dec)
	{
		int pow;
		char rest;
		pow = 65536;
		rest = 0;

		while (rest == 0 && pow > 0)
		{
			rest = (char)floor(Dec / pow);
			if (rest == 0)
				pow = floor(pow / 2);
		}
		while (pow > 1)
		{
			Dec = Dec - pow * rest;
			pow = floor(pow / 2);
			rest = (char)floor(Dec / pow);
			NoSeq++;
			cout << NoSeq;
			Sequence[NoSeq] = rest;
		}
		cout << '\n';
		return 0;
	}

	void Draw(float Seed, int NoIter, int RozX, int RozY, int start, int koniec, float RXMin, float RXMax, float RYMin, float RYMax, int val, int s)
	{
		float rx, ry, deltaX, deltaY, tmpLap = 0;
		int k, w;
		char tmp;

		for (k = 0; k < 16; k++)
			Sequence[k] = 0;

		sample = s;
		NoSeq = 0;
		Sequence[0] = 1;
		DecToBin(val);
		LapMin = -2;
		MinR = RXMin;
		MaxR = RXMax;
		MinX = RYMin;
		MaxX = RYMax;
		deltaX = (MaxR - MinR) / RozX;
		deltaY = (MaxX - MinX) / RozY;
		rx = MinR;
		ry = MaxX - (start - 1) * deltaY;
		double z;
		for (k = 0; k < 15; k++)
			std::cout << (int)Sequence[k];

		std::cout << "\n";

		// tbb
		int gs_wiersze = 16;
		int gs_kolumny = 32;
		tbb::parallel_for(
			tbb::blocked_range2d<size_t>(0, RozX, gs_wiersze, 0, RozY, gs_kolumny ),
			[=](const tbb::blocked_range2d<size_t> &r)
			{
				float rx = 0;
				float ry = 0;
				float tmpLap = 0;
				float z;
				char tmp;

				float rxstart = MinR;
				float rystart = MaxX - (start - 1) * deltaY;

				for (int w = r.rows().begin(); w < r.rows().end(); w++)
				{
					for (int k = r.cols().begin(); k < r.cols().end(); k++)
					{
						float tmpLap = ValLap(Seed, NoIter, rx, ry);
						auto id_watka = tbb::task_arena::current_thread_index();
						
						if (tmpLap <= 0)
						{
							z = noColors * tmpLap / LapMin;
							tmp = (int)(z) % noColors;
							pixels[k][w][0] = tmp;
							pixels[k][w][1] = tmp;
							pixels[k][w][2] = tmp;
						}
						else
						{

							pixels[k][w][0] = colors[id_watka][0];
							pixels[k][w][1] = colors[id_watka][1];
							pixels[k][w][2] = colors[id_watka][2];
						}
						rx = rxstart + deltaX * k;
					}
					ry = rystart - (deltaY * w);
				}
			});
	}

	float ValLap(float Seed, int NoIter, float rx, float ry)
	{
		float x, sumlap, elem, ValLap;
		int i, poz, NoElem;
		float R;

		x = Seed;
		sumlap = 0;
		NoElem = 0;
		poz = 0;

		for (i = 1; i <= NoIter; i++)
		{
			if (Sequence[poz] == 0)
				R = ry;
			else
				R = rx;
			poz++;
			if (poz > NoSeq)
				poz = 0;
			x = fun(x, R);
			elem = (float)abs(dfun(x, R));
			if (elem > 1000)
			{
				ValLap = 10;
				break;
			}
			if (elem != 0)
			{
				sumlap = sumlap + (float)log2(elem);
				NoElem++;
			}
		}
		if (NoElem > 0)
			ValLap = sumlap / NoElem;
		else
			ValLap = 0;
		return ValLap;
	}

	float fun(float x, float r)
	{
		float y = 0;
		switch (sample)
		{
		case (0):
			y = r * sin(x) + r;
			break;
		case (1):
			y = r * cos(x) + r;
			break;
		case (2):
			y = r * cos(x) * (1 - sin(x));
			break;
		}
		return y;
	}

	float dfun(float x, float r)
	{
		float y = 0;
		switch (sample)
		{
		case (0):
			y = r * cos(x);
			break;
		case (1):
			y = -r * sin(x);
			break;
		case (2):
			y = r * (1 - sin(x)) - 2 * cos(x) * cos(x);
			break;
		}
		return y;
	}
};

int main(int ac, char ** av)
{
	int th = 4;
	if (ac > 1)
	{
		th = stoi(av[1]);
		cout << "th = " << th << endl;
	}
	tbb::task_scheduler_init init(th);

	FILE *fp;
	int i, j;
	char *filename = "new1.ppm";
	char *comment = "# "; // comment should start with # 
	const int MaxColorComponentValue = 255;
	
	int a;
	Lapunov lp;
	fp = fopen(filename, "wb"); /* b -  binary mode */
	/*write ASCII header to the file*/

	fprintf(fp, "P6\n %s\n %d\n %d\n %d\n", comment, size, size, MaxColorComponentValue);

	tbb::tick_count time_start, time_end;
	time_start = tbb::tick_count::now();

	lp.Draw(5, 100, size, size, 0, size, -3, 9, -5, 2, 2477, 1);

	time_end = tbb::tick_count::now();
	cout << "Time: " << (time_end - time_start).seconds() << endl;

	fwrite(pixels, 1, 3 * size * size, fp);

	fclose(fp);

	return 0;
}