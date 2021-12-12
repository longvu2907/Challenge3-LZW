#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <math.h>

//Giới hạn từ điển 12 bit 2^12 = 4096
#define DICT_SIZE 4095

using namespace std;

int dictSize = 9;

//Hàm mã hóa và giải mã tham khảo tại: https://www.geeksforgeeks.org/lzw-lempel-ziv-welch-compression-technique/
//
//Mã hóa dữ liệu cần nén
vector<int> encoding(string s, bool dMode) {
	unordered_map<string, int> dict;
	//Khởi tạo từ điển ban đầu chứa bảng mã ascii
	for (int i = 0; i <= 255; i++) {
		string ch = "";
		ch += char(i);
		dict[ch] = i;
	}

	string p = "", c = "";
	p += s[0]; //Khởi tạo p là kí tự đầu
	int code = 256; //code là giá trị mở rộng đầu tiên của từ điển
	vector<int> outputCode;
	if (dMode) cout << "\n- Additional Dictionary:\n";

	//Duyệt qua từng kí tự
	for (int i = 0; i < s.length(); i++) {

		if (i != s.length() - 1)
			c += s[i + 1]; //c là kí tự tiếp theo

		//Kiểm tra p + c đã tồn tại trong từ điển chưa
		if (dict.find(p + c) != dict.end()) 
			//Nếu đã tồn tại thì gán p = p + c
			p = p + c;
		else {
			//Nếu chưa tồn tại thì thêm mã của p vào outputCode
			outputCode.push_back(dict[p]);
			if (dict.size() > (pow(2, dictSize) - 1)) dictSize += 1;
			//Nếu chưa vượt quá giới hạn 2^12 bit thì mở rộng từ điển: p + c với mã code
			dict[p + c] = code;
			if (dMode) cout << p + c << ": " << code << endl; //In ra từ cùng mã của nó mới được tạo ra.
			code++; //tăng mã code lên 1
			p = c; //Cập nhật lại p
		}
		c = ""; //Cập nhật lại c
	}
	outputCode.push_back(dict[p]);
	return outputCode;
}
//Giải mã dữ liệu
string decoding(vector<int> code, bool dMode) {
	string res = ""; //Khởi tạo kết quả sau khi giải mã
	unordered_map<int, string> table;
	//Khởi tạo từ điển ban đầu chứa bảng mã ascii
	for (int i = 0; i <= 255; i++) {
		string ch = "";
		ch += char(i);
		table[i] = ch;
	}

	int old = code[0], n; //old kí tự đầu tiên.
	string s = table[old]; //Giải mã kí tự đầu tiên.
	string c = "";
	c += s[0];
	res += s; //Thêm kí tự đầu tiên đã giải mã vào kết quả
	int count = 256;
	if (dMode) cout << "\n- Additional Dictionary:\n";

	//Duyệt qua từng kí tự
	for (int i = 0; i < code.size() - 1; i++) {
		n = code[i + 1];//n là kí tự tiếp theo
		if (table.find(n) == table.end()) {
			//Nếu n chưa tồn tại trong từ điển
			s = table[old]; //gán s là kí tự được giải nén của old
			s = s + c; //gán s = s + c
		}
		else {
			//Nếu n đã tồn tại
			s = table[n]; //gán s là kí tự được giải mã của n
		}
		res += s; //thêm vào kết quả chuỗi s đã giải mã

		//cập nhật lại c
		c = ""; 
		c += s[0]; 

		table[count] = table[old] + c; //Mở rộng từ điển

		if (dMode) cout << table[old] + c << ": " << count << endl; //In ra từ cùng mã của nó mới được tạo ra.

		count++;
		old = n; //cập nhật lại old
	}
	return res;
}

//Tham khảo tại: https://github.com/radekstepan/LZW
// 
//Cách thức hoạt động của hàm ghi file binary chia mỗi mã input từ 12 bits thành 8 bits đầu
//và lưu 4 bits cuối cho lần ghi tiếp theo.
//Nếu có 4 bits từ lần ghi trước đó thì ghép 4 bits đó với 12 bits hiện tại và ghi vào file
void writeBinary(ofstream& output, int code, int& leftover, int& leftoverBits, long& size) {
	int n = (dictSize - 8);
	if (leftover > 0) {
		//Nếu lần trước có dư ra 4 bits

		//previous code = 4 bits cuối của lần ghi trước + với 
		//4 bits đầu của input hiện tại
		int mask = (1 << leftover) - 1;
		leftoverBits = code & mask; //lưu lại 4 bits cuối
		if (leftover == 8) leftover = 0; //đánh dấu dư 4 bits
		else leftover += 1;
		int previousCode = (leftoverBits << (8-leftover)) + (code >> leftover);

		//ghi ra file 4 bits cuối của lần ghi trước cùng với 12 bits hiện tại
		output.write((char*)&previousCode, 1);
		output.write((char*)&code, 1);
		size += 2;

		leftover = 0; //cập nhật lại leftover
	}
	if (leftover == 0) {
		int mask = (1 << n) - 1;
		leftoverBits = code & mask; //lưu lại 4 bits cuối
		leftover = 1; //đánh dấu dư 4 bits
		
		//Ghi 8 bits đầu vào file
		code = code >> n;
		output.write((char*)&code, 1);
		size += 1;
	}
}
//Cách thức hoạt động của hàm đọc file binary: đọc luân phiên 16 bits và 8 bits.
//lần đọc 16 bits: đọc 12 bits đầu và lưu 4 bits cuối lại cho lần đọc sau.
//lần đọc 8 bits: ghép với 4 bits từ lần đọc trước cùng với 8 bits hiện tại tạo thành 12 bits.
int readBinary(ifstream& input, int& leftover, int& leftoverBits) {
	int code = input.get(); //đọc 8 bits đầu
	if (code == EOF) return 0; //Nếu kết thúc file return

	if (leftover > 0) {
		//Nếu lần đọc trước còn dư 4 bits
		//đọc 4 bits lần trước cùng với 8 bits hiện tại
		code = (leftoverBits << 8) + code;

		leftover = 0;
	}
	else {
		int nextCode = input.get(); //đọc 8 bits tiếp theo

		leftoverBits = nextCode & 0xF; //lưu lại 4 bits cuối 
		leftover = 1; //đánh dấu dư 4 bits

		//đọc 12 bits đầu còn dư lại 4 bits
		code = (code << 4) + (nextCode >> 4);
	}
	return code;
}

//Hàm nén file
void compress(string inputFileName, string outputFileName, bool iMode = false, bool dMode = false) {
	cout << "Compressing..." << endl;
	ifstream fin(inputFileName);

	if (!fin) {
		cout << "Input file not exists" << endl;
		exit(0);
	}

	fin.seekg(0, fin.end);
	long finSize = fin.tellg();
	fin.seekg(0, fin.beg);

	stringstream stream;
	stream << fin.rdbuf();
	string s = stream.str();
	vector<int> a = encoding(s, dMode);
	fin.close();

	ofstream fout(outputFileName, ios::binary);
	int leftover = 0;
	int leftoverBits;
	long foutSize = 0;
	for (auto i : a) {
		writeBinary(fout, i, leftover, leftoverBits, foutSize);
	}
	if (leftover > 0) {
		leftoverBits = leftoverBits << 4;
		fout.write((char*)&leftoverBits, 1);
		foutSize += 1;
	}

	if (iMode) {
		cout << "\n- Input Size, Output Size:\n";
		cout << "Input size: " << finSize * 8 << " bits" << endl;
		cout << "Output size: " << foutSize * 8 << " bits" << endl;
		cout.precision(2);
		cout << "Space saved: " << (double(finSize - foutSize) / finSize) * 100 << "%" << endl;
	}
}

//Hàm giải nén file
void decompress(string inputFileName, string outputFileName, bool iMode = false, bool dMode = false) {
	cout << "Decompressing..." << endl;
	ifstream fin(inputFileName, ios::binary);

	if (!fin) {
		cout << "Input file not exists" << endl;
		exit(0);
	}

	fin.seekg(0, fin.end);
	long finSize = fin.tellg();
	fin.seekg(0, fin.beg);

	vector<int> outputCode;
	int code;
	int leftover = 0;
	int leftoverBits;
	while ((code = readBinary(fin, leftover, leftoverBits)) > 0) {
		outputCode.push_back(code);
	}
	ofstream fout(outputFileName);
	string resString = decoding(outputCode, dMode);
	fout << resString;

	long foutSize = resString.size();

	if (iMode) {
		cout << "\n- Input Size, Output Size:\n";
		cout << "Input size: " << finSize * 8 << " bits" << endl;
		cout << "Output size: " << foutSize * 8 << " bits" << endl;
		cout.precision(2);
		cout << "Space saved: " << (double(foutSize - finSize) / foutSize) * 100 << "%" << endl;
	}
}

//In ra hướng dẫn sử dụng
void usage() {
	cout << "<Command>: LZW.exe Action Inputpath Outputpath Outputinfo" << endl;
	cout << "+ Action:" << endl;
	cout << "\t-e: Nen file." << endl;
	cout << "\t-d: Giai nen file." << endl;
	cout << "+ Inputpath: Duong dan den file input (-e: .txt, -d: .lzw)." << endl;
	cout << "+ Outputpath: Duong dan den file output (-e: .lzw, -d: .txt)." << endl;
	cout << "+ Outputinfo: " << endl;
	cout << "\t-i: Kich thuoc cua file input, kich thuoc cua file output, ti le khong gian bo nho tiet kiem." << endl;
	cout << "\t-d: Tu dien LZW duoc tao ra." << endl;
	cout << "\t-ind: Ca 2 thong tin tren." << endl;
	exit(0);
}

int main(int argc, char** argv) {
	////Nếu không đủ 5 tham số hiện hướng dẫn và thoát chương trình
	if (argc != 5) usage();
	string actionmode = argv[1];

	if (actionmode == "-e") {
		//nén
		string inputpath = argv[2];
		string outputpath = argv[3];

		//kiểm tra tên file hợp lệ
		if (inputpath.find(".txt") != string::npos && outputpath.find(".lzw") != string::npos) {
			string outputinfo = argv[4];
			bool imode = false, dmode = false;

			//kiểm tra thông tin output
			if (outputinfo == "-i") imode = true;
			else if (outputinfo == "-d") dmode = true;
			else if (outputinfo == "-ind") imode = dmode = true;
			else usage();

			compress(inputpath, outputpath, imode, dmode);
		}
		else usage();
	}
	else if (actionmode == "-d") {
		//giải nén
		string inputpath = argv[2];
		string outputpath = argv[3];

		//kiểm tra tên file hợp lệ
		if (inputpath.find(".lzw") != string::npos && outputpath.find(".txt") != string::npos) {
			string outputinfo = argv[4];
			bool imode = false, dmode = false;

			//kiểm tra thông tin output
			if (outputinfo == "-i") imode = true;
			else if (outputinfo == "-d") dmode = true;
			else if (outputinfo == "-ind") imode = dmode = true;
			else usage();

			decompress(inputpath, outputpath, imode, dmode);
		}
		else usage();
	}
	else usage();

	return 0;
}
