#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<cstdio>
#include<fstream>
#include<istream>
#include<string.h>
#include<string>
#include<sstream>
#include<math.h>
using namespace std;

int regV[32] = { 0 };	//32個暫存器的值		regV[regN[2][0]]=regV[2]
int memory[32] = { 0 };  //32個WORD的記憶體
int regAdd = 0;          //*EXE執行完傳給WB存給reg
int regSub = 0;			 //*
int regLoad = 0;		 //*
int HowManyLines = 0;	 //讀檔時計算總共有幾個指令
string temp;			 //暫存讀進來的一行
int *instr;				//動態  存指令
int *rs;				//	    存rs
int *rt;				//		存rt
int *rd;				//		存rd
int *offset;			//		存offset
int *stall;				//		存stall
int *Jflag;				//		判斷beq是否跳躍 0:沒跳 1:有跳
int *JumpToflag;		//		若無跳 判斷上上個指令
string *dataA;			//		把讀檔的東西放進此陣列裡
int compute=0;			//      暫存計算過的值 再放進對應的陣列裡
bool memoryflag = false;//		是否有進出mem
bool jump = false;		//		判斷是否有跳
int signal[7] = { 0 };  //		ControlSignal
string *temp2;			//		有執行就放進一行 方便判斷		
double L;				//		字串長度
string stage[6] = { "","IF","ID","EXE","MEM","WB" };
string instruction[6] = { "","add","sub","lw","sw","beq" };
int stageflag[5] = { 0 };
int controlRtype[7];	//signal 型態
int controllw[7];		//*
int controlsw[7];		//*

void reset_regW()
{
	for (int i = 1; i<32; i++)	//初始化 1~31=1		regV[0~31]
		regV[i] = 1;

	for (int i = 0; i<32; i++)
		memory[i] = 1;
}
void readfile() {
	fstream fin1;	//file input "fin"
	fin1.open("memory.txt", ios::in);		//讀取檔案
											//讀取 將程式碼放入a字串
	while (!fin1.eof())
	{
		getline(fin1, temp);
		HowManyLines++;
	}
	fin1.close();
	//-----------------------------------------------
	instr = new int[HowManyLines*10];
	rs = new int[HowManyLines*10];
	rt = new int[HowManyLines*10];
	rd = new int[HowManyLines*10];
	offset = new int[HowManyLines*10];
	stall = new int[HowManyLines*10];
	temp2 = new string[HowManyLines*10];
	dataA = new string[HowManyLines * 10];
	Jflag = new int[HowManyLines * 10];
	JumpToflag = new int[HowManyLines * 10];
	for (int i = 0; i<HowManyLines*10; i++) {
		rs[i] = 0;
		rt[i] = 0;
		rd[i] = 0;
		stall[i] = 0;
		offset[i] = 0;
		instr[i] = 0;
		Jflag[i] = 0;
		JumpToflag[i] = 0;
	}
}
//void IF(fstream &fin, int count)			//需要 '&'
void IF(int next_i, int count)
{
	//getline(fin, temp2[count]);
	//cout<<temp2[count]<<endl;
	//cout << dataA[count];
	temp2[next_i] = dataA[count];
	//cout << temp2[next_i] << endl;
	//cout <<"下一個位置"<< next_i << endl;
}

void ID(int i)
{
	stringstream ss(temp2[i]);
	string sub_str;
	int circle = 0;
	while (getline(ss, sub_str, ' ')) {
		//cout<<sub_str<<endl;
		if (circle == 0) {													//instr  判斷
			if (sub_str == "add")  instr[i] = 1;
			if (sub_str == "sub")  instr[i] = 2;
			if (sub_str == "lw")  instr[i] = 3;
			if (sub_str == "sw")  instr[i] = 4;
			if (sub_str == "beq")  instr[i] = 5;
		}
		if (circle == 1) {													//第一個位置的判斷
			if (instr[i] == 1 || instr[i] == 2) {								//R-type 存進rd
				L = sub_str.length();
				compute = 0;
				for (int x = L - 2; x>0; x--) {
					compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 2));
				}
				rd[i] = compute;
			}
			if (instr[i] == 3 || instr[i] == 4 || instr[i] == 5) {			//lw,sw,beq 存進rs
				L = sub_str.length();
				compute = 0;
				for (int x = L - 2; x > 0; x--) {
					compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 2));
				}
				rs[i] = compute;
			}
		}
		if (circle == 2) {														//第二個位置的判斷
			if (instr[i] == 1 || instr[i] == 2) {								//R-type 存進rs
				L = sub_str.length();
				compute = 0;
				for (int x = L - 2; x>0; x--) {
					compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 2));
				}
				rs[i] = compute;
			}
			if (instr[i] == 3 || instr[i] == 4) {								//lw,sw 再切割
				stringstream ff(sub_str);
				string sub_str2;
				int s = 0;
				while (getline(ff, sub_str2, '(')) {
					switch (s) {
					case 0:												//存進offset
						L = sub_str2.length();
						compute = 0;
						for (int x = L - 1; x >= 0; x--) {
							compute = compute + ((int)sub_str2[x] - 48)* pow(10, (L - x - 1));
						}
						offset[i] = compute;
						break;
					case 1:												//存進rt
						L = sub_str2.length();
						compute = 0;
						for (int x = L - 2; x>0; x--) {
							compute = compute + ((int)sub_str2[x] - 48)* pow(10, (L - x - 2));
						}
						rt[i] = compute;
						break;
					}
					s++;
				}
			}
			if (instr[i] == 5) {
				L = sub_str.length();
				compute = 0;
				for (int x = L - 2; x>0; x--) {
					compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 2));
				}
				rt[i] = compute;
			}
		}
		if (circle == 3) {													//第三個位置的判斷
			if (instr[i] == 1 || instr[i] == 2) {								//R-type 存進rt
				L = sub_str.length();
				compute = 0;
				for (int x = L - 1; x>0; x--) {
					compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 1));
				}
				rt[i] = compute;
			}
			if (instr[i] == 5) {												//beq 存進offset
				if (sub_str[0] == '-') {													//判斷正負
					L = sub_str.length();
					compute = 0;
					for (int x = L - 1; x>0; x--) {
						compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 1));
					}
					offset[i] = compute;
					offset[i] = offset[i] * -1;
				}
				else {
					L = sub_str.length();
					compute = 0;
					for (int x = L - 1; x >= 0; x--) {
						compute = compute + ((int)sub_str[x] - 48)* pow(10, (L - x - 1));
					}
					offset[i] = compute;
				}
			}
			circle = 0;
		}
		circle++;
	}
	//--------------------------------------------------------------------------------------------判斷是否有stall
	if (jump)stall[i] = 0;
	else if (i>0) {
		switch (instr[i]) {
		case 1://add
			if (instr[i - 1] == 1) {								//前指令是add
				if (rd[i - 1] == rs[i] || rd[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 2) {								//前指令是sub
				if (rd[i - 1] == rs[i] || rd[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 3) {								//前指令是lw
				if (rs[i - 1] == rs[i] || rs[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 4) { stall[i] = 0;	}				//前指令是sw
			break;

		case 2://sub
			if (instr[i - 1] == 1) {								//前指令是add
				if (rd[i - 1] == rs[i] || rd[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 2) {								//前指令是sub
				if (rd[i - 1] == rs[i] || rd[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 3) {								//前指令是lw
				if (rs[i - 1] == rs[i] || rs[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 4) { stall[i] = 0; 	}				//前指令是sw
			break;
		case 3://lw
			if (instr[i - 1] == 1) {								//前指令是add
				if (rd[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 2) {								//前指令是sub
				if (rd[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 3) {								//前指令是lw
				if (rs[i - 1] == rt[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 4) {								//前指令是sw
				if ((rt[i - 1] + offset[i - 1] / 4) == (rt[i] + offset[i] / 4))
					stall[i] = 1;
			}
			break;
		case 4://sw
			if (instr[i - 1] == 1) {								//前指令是add
				if (rd[i - 1] == rs[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 2) {								//前指令是sub
				if (rd[i - 1] == rs[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 3) {								//前指令是lw
				if (rs[i - 1] == rs[i])
					stall[i] = 2;
			}
			if (instr[i - 1] == 4) stall[i] = 0;					//前指令是sw
			break;
		case 5://beq
			if (instr[i - 1] == 1) {								//前指令是add
				if ((rd[i - 1] == rs[i]) || (rd[i - 1] == rt[i])) {
					stall[i] = 2;	
				}
				else {
					if ((instr[i - 2] == 1) || (instr[i - 2] == 2))
						if ((rd[i - 2] == rs[i]) || (rd[i - 2] == rt[i]))
							stall[i] = 1;
					if (instr[i - 2] == 3)
						if ((rs[i - 2] == rs[i]) || (rs[i - 2] == rt[i]))
							stall[i] = 1;
					}
				}

			if (instr[i - 1] == 2) {								//前指令是sub
				if ((rd[i - 1] == rs[i]) || (rd[i - 1] == rt[i])) {
					stall[i] = 2;
				}
				else {
					if ((instr[i - 2] == 1) || (instr[i - 2] == 2))
						if ((rd[i - 2] == rs[i]) || (rd[i - 2] == rt[i]))
							stall[i] = 1;
					if (instr[i - 2] == 3)
						if ((rs[i - 2] == rs[i]) || (rs[i - 2] == rt[i]))
							stall[i] = 1;
				}
			}

			if (instr[i - 1] == 3) {								//前指令是lw
				if ((rs[i - 1] == rs[i])|| (rs[i - 1] == rt[i]))
					stall[i] = 2;
				else {
					if ((instr[i - 2] == 1) || (instr[i - 2] == 2))
						if ((rd[i - 2] == rs[i]) || (rd[i - 2] == rt[i]))
							stall[i] = 1;
					if (instr[i - 2] == 3)
						if ((rs[i - 2] == rs[i]) || (rs[i - 2] == rt[i]))
							stall[i] = 1;
				}
			}
			if (instr[i - 1] == 4) stall[i] = 0;					//前指令是sw
			break;
		}
	}
}

void EXE(int i)
{
	switch (instr[i]) {
	case 1:
		regAdd = regV[rs[i]] + regV[rt[i]];
		jump = false;
		break;
	case 2:
		regSub = regV[rs[i]] - regV[rt[i]];
		jump = false;
		break;
	case 3:
		memoryflag = true;
		jump = false;
		break;
	case 4:
		memoryflag = true;
		jump = false;
		break;
	case 5:
		if (regV[rt[i]] - regV[rs[i]] == 0) {
			//i = i + 1 + rd[i];			
			jump = true;
		}
		else {
			Jflag[i + 1] = 2;
			//jump = false;
		}
		
		break;
	}
}

void MEM(int i)
{
	if (memoryflag = true) {
		if (instr[i] == 3) {
			regLoad = memory[rt[i] + (offset[i] / 4)];
		}
		if (instr[i] == 4) {
			memory[rt[i] + (offset[i] / 4)] = regV[rs[i]];
		}
	}
}

void WB(int i)
{
	if (instr[i] == 1) {
		regV[rd[i]] = regAdd;
	}
	if (instr[i] == 2) {
		regV[rd[i]] = regSub;
	}
	if (instr[i] == 3) {
		regV[rs[i]] = regLoad;
	}
}

int main()
{
	reset_regW();
	readfile();

	fstream fin333;	//file input "fin"
	fin333.open("memory.txt", ios::in);		//讀取檔案
	int coco = 0;
	while (!fin333.eof())
	{
		getline(fin333, dataA[coco]);
		//cout << dataA[coco] << endl;
		coco++;
		//cout << coco << endl;
	}
	fin333.close();
	//for (int i = 0; i < coco; i++) { cout << dataA[i] << endl; }

	//fstream fin;	//file input "fin"
	//fin.open("memory.txt", ios::in);		//讀取檔案
	fstream fout;	//file output "fout"
	fout.open("result.txt", ios::out);	//清空,再輸入內容
										//讀取 將程式碼放入a字串
	int next_i = 0;
	for (int i = 0; i < HowManyLines * 10; i++) {				//-----------MIPS模擬執行
		//cout << "**"<<i << endl;
		
		IF(next_i, i);
		ID(next_i);
		EXE(next_i);
		if (jump == true) {
			JumpToflag[next_i + 2] = 2;
			next_i = next_i + 1; 			
			IF(next_i, i + 1);
			ID(next_i);
			stall[next_i] = 0;
			Jflag[next_i] = 1;
			i = i + offset[i]; 

			//cout << "jump " << i << endl; 
			//jump = false; 
			next_i = next_i + 1;
			continue; }
		MEM(next_i);
		WB(next_i);
		next_i = next_i+1;
	}
	int totalstall = 0;
	int Cycles = 0;
	int countC = 0;//總共有幾行
	for (int k = 0; k < HowManyLines * 10; k++) {
		if (temp2[k] != "") {
			countC++;
		}
		else { break; }
	}
	//cout << countC << endl;
	for (int j = 0; j < countC; j++) {
		totalstall = totalstall + stall[j];
	}
	Cycles = 5 - 1 + countC + totalstall;			// 計算總共有幾行 來改HowmanyLines
	for (int l = 0; l < HowManyLines; l++) {
		if (Jflag[l] == 2)
			Cycles++;
	}
	int **output;
	int **output1;
	output = new int *[countC];
	output1 = new int *[countC];

	for (int i = 0; i < countC; i++) {
		output[i] = new int[Cycles];
		output1[i] = new int[Cycles];
		for (int j = 0; j < Cycles; j++) {
			output[i][j] = 0;
			output1[i][j] = 0;
		}
	}
	int ci = 0;
	for (int i = 0; i < countC; i++)
	{
		int c1 = i;		
		//cout << i << ":" << JumpToflag[i] << endl;
		for (int i2 = 1; i2 < 6; i2++)
		{
			output[i][c1] = i2;
			if (i == 0) { c1++; continue; }		
			if (Jflag[i] == 1) {
				if (output[i - 1][c1] == 3) {
					output[i][c1] = 0;
					break; }
			}
			if (Jflag[i] == 2) {
				if (output[i - 1][c1] < 3 && output[i-1][c1] !=0) {
					output[i][c1] = 1;
					i2--;
					c1++;
					continue;
				}
			}
			if (JumpToflag[i] == 2) {
				if (output[i - 2 ][c1] == 3) {
					output[i][c1] = 1;
					i2 = 1;
					c1++;
					continue;
				}
				if(output[i-2][c1]>3 && output[i-2][c1] != 0){
					output[i][c1] = i2;
					c1++;
					continue;
				}
			}

			if (output[i - 1][c1] == 0 && i2 == 1) {
				output[i][c1] = 0;
				i2--;
				c1++;
				continue;
			}
			else if (output[i - 1][c1] == 1) {
				output[i][c1] = 0;
				i2--;
				c1++;
				continue;
			}

			if (i2 == 2 && output[i - 1][c1] == 2) { 
				output[i][c1] = 1;
				i2--;
			}
			else if (i2 == 2 && stall[i] != 0) {
				output[i][c1] = 2;
				i2--;
				stall[i]--;
			}
			c1++;
		}
	}

	for (int i = 0; i < Cycles; i++) {
		cout << "Cycle " << i + 1 << endl;
		fout << "Cycle " << i + 1 << endl;
		for (int j = 0; j < countC; j++) {
			if (output[j][i] == 0) {
				continue;
			}
			else {
				switch (instr[j])
				{
				case 1:
					cout << "	" << instruction[1] << ":" << stage[output[j][i]] ;
					fout << "	" << instruction[1] << ":" << stage[output[j][i]];
					if (output[j][i] == 3)//EX
					{
						cout << " 10 000 10";
						fout << " 10 000 10";
					}
					if (output[j][i] == 4)//MEM
					{
						cout << " 000 10";
						fout << " 000 10";
					}
					if (output[j][i] == 5)//WB
					{
						cout << " 10";
						fout << " 10";
					}

					cout << endl;
					fout << endl;
					break;
				case 2:
					cout << "	" << instruction[2] << ":" << stage[output[j][i]] ;
					fout << "	" << instruction[2] << ":" << stage[output[j][i]] ;
					if (output[j][i] == 3)//EX
					{
						cout << " 10 000 10";
						fout << " 10 000 10";
					}
					if (output[j][i] == 4)//MEM
					{
						cout << " 000 10";
						fout << " 000 10";
					}
					if (output[j][i] == 5)//WB
					{
						cout << " 10";
						fout << " 10";
					}
					cout << endl;
					fout << endl;
					break;
				case 3:
					cout << "	" << instruction[3] << ":" << stage[output[j][i]] ;
					fout << "	" << instruction[3] << ":" << stage[output[j][i]] ;

					if (output[j][i] == 3)//EX
					{
						cout << " 01 010 11";
						fout << " 01 010 11";
					}
					if (output[j][i] == 4)//MEM
					{
						cout << " 010 11";
						fout << " 010 11";
					}
					if (output[j][i] == 5)//WB
					{
						cout << " 11";
						fout << " 11";
					}
					cout << endl;
					fout << endl;

					break;
				case 4:
					cout << "	" << instruction[4] << ":" << stage[output[j][i]] ;
					fout << "	" << instruction[4] << ":" << stage[output[j][i]];
					if (output[j][i] == 3)//EX
					{
						cout << " X1 001 0X";
						fout << " X1 001 0X";
					}
					if (output[j][i] == 4)//MEM
					{
						cout << " 001 0X";
						fout << " 001 0X";
					}
					if (output[j][i] == 5)//WB
					{
						cout << " 0X";
						fout << " 0X";
					}

					cout << endl;
					fout << endl;
					break;
				case 5:
					cout << "	" << instruction[5] << ":" << stage[output[j][i]];
					fout << "	" << instruction[5] << ":" << stage[output[j][i]];
					if (output[j][i] == 3)//EX
					{
						cout << " X0 100 0X";
						fout << " X0 100 0X";
					}
					if (output[j][i] == 4)//MEM
					{
						cout << " 100 0X";
						fout << " 100 0X";
					}
					if (output[j][i] == 5)//WB
					{
						cout << " 0X";
						fout << " 0X";
					}
					cout << endl;
					fout << endl;
					break;
				}

			}
		}
	}
	cout<< endl;
	fout << endl;
	/*for (int i = 0; i < countC; i++) {
		for (int j = 0; j < Cycles; j++) {
			cout << " " << output[i][j];
		}
		cout << endl;
	}*/

	/*for (int i = 0; i<countC; i++) {
		cout << "instr:" << instr[i] << " rd:" << rd[i] << " rs:" << rs[i] << " rt:" << rt[i] << " offset:" << offset[i] << " stall:" << stall[i] << endl;
	}*/

	for (int i = 0; i<32; i++) {
		printf("R[%2d]：%2d ", i, regV[i]); 
		fout << "R[" << i << "]:" << regV[i]<<" ";
		if ((i + 1) % 8 == 0) { cout << endl; fout << endl; }
	}
	cout << endl;
	cout <<"-----------------------------------------------------------------"<< endl;
	fout << "-----------------------------------------------------------------" << endl;
	for (int i = 0; i<32; i++) {
		printf("M[%2d]：%2d ", i, memory[i]);
		fout << "M[" << i << "]:" << memory[i] << " ";
		if ((i + 1) % 8 == 0) { cout << endl; fout << endl; }
	}

	fout.close();
	cout << endl;
	system("pause");
	return 0;
}