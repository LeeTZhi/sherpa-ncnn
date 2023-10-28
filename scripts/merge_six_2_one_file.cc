/*
 Read 6 files and merge them into one file.
 file saved:
  Magic number: 0xA1A2A3A4
  FileHader 1: 2 bytes(file type: EP, EB, DP, DB, EP, EB or JP, JB) + 4 bytes(file size) + data
*/
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

//1 byte alignment
#pragma pack(1)
typedef struct file_header {
  char file_id[2];
  uint32_t file_size;
} file_header;

/// load dp file and load the merged file, then compare them
bool load_and_verify(const string& dp_file_name, const string& merged_file_name) {
    ifstream dp_file(dp_file_name.c_str(), ios::binary);
    if (!dp_file) {
        cout << "Failed to open file: " << dp_file_name << endl;
        return false;
    }
    
    ifstream merged_file(merged_file_name.c_str(), ios::binary);
    if (!merged_file) {
        cout << "Failed to open file: " << merged_file_name << endl;
        return false;
    }
    
    //Get file size
    dp_file.seekg(0, ios::end);
    uint32_t dp_file_size = dp_file.tellg();
    dp_file.seekg(0, ios::beg);
    
    char* dp_buf = new char[dp_file_size];

    ///read magic number
    uint32_t magic_number = 0;
    merged_file.read((char*)&magic_number, sizeof(magic_number));
    //read DP file header and data
    file_header fh;
    merged_file.read((char*)&fh, sizeof(fh));
    while (memcmp(fh.file_id, "DP", 2) != 0) {
        merged_file.seekg(fh.file_size, ios::cur);
        merged_file.read((char*)&fh, sizeof(fh));
    }
    ///if eof is reached, return false
    if (merged_file.eof()) {
        cout << "Failed to find DP file in merged file" << endl;
        ///close all files
        dp_file.close();
        merged_file.close();
        return false;
    }
    ///compare size first
    if (fh.file_size != dp_file_size) {
        cout << "File size not match" << endl;
        ///close all files
        dp_file.close();
        merged_file.close();
        return false;
    }

    char* merged_buf = new char[fh.file_size];

    dp_file.read(dp_buf, dp_file_size);
    merged_file.read(merged_buf, fh.file_size);
    
    //xor buf with magic number
    for (int j = 0; j < dp_file_size; ++j) {
        merged_buf[j] ^= (magic_number >> ((j % 4) * 8)) & 0xFF;
    }
    
    if (memcmp(dp_buf, merged_buf, dp_file_size) != 0) {
        cout << "File content not match" << endl;
        return false;
    }
    
    dp_file.close();
    merged_file.close();

    delete[] dp_buf;
    delete[] merged_buf;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 8) {
        cout << "Usage: " << argv[0] << " EN_file_param EN_file_bin "
             << "DE_file_param DE_file_bin output_file" <<
             "JO_file_param JO_file_bin"
             << endl;
        return -1;
    }
    
    string file_name[6];
    for (int i = 0; i < 6; ++i) {
        file_name[i] = argv[i + 1];
    }
    const char* file_type[6] = {"EP", "EB",  "DP", "DB", "JP", "JB"};

    string output_file = argv[7];
    
    ofstream fout(output_file.c_str(), ios::binary);
    if (!fout) {
        cout << "Failed to open file: " << output_file << endl;
        return -1;
    }
    
    // Write magic number
    uint32_t magic_number = 0xA1A2A3A4;
    fout.write((char*)&magic_number, sizeof(magic_number));
    
    // Write file header
    file_header fh;
    for (int i = 0; i < 6; ++i) {
        ifstream fin(file_name[i].c_str(), ios::binary);
        if (!fin) {
        cout << "Failed to open file: " << file_name[i] << endl;
        return -1;
        }
    
        //Get file size
        fin.seekg(0, ios::end);
        fh.file_size = fin.tellg();
        fin.seekg(0, ios::beg);
        //set file type
        memcpy(fh.file_id, file_type[i], 2);
        //Write file header
        fout.write((char*)&fh, sizeof(fh));
        //Write file data
        char* buf = new char[fh.file_size];
        fin.read(buf, fh.file_size);
        //xor buf with magic number
        for (int j = 0; j < fh.file_size; ++j) {
            buf[j] ^= (magic_number >> ((j % 4) * 8)) & 0xFF;
        }
        fout.write(buf, fh.file_size);
        delete[] buf;
    }
    
    fout.close();

    ///checkout the merged file
    if (load_and_verify(file_name[2], output_file)) {
        cout << "Merge file successfully" << endl;
    } else {
        cout << "Merge file failed" << endl;
    }
    return 0;
}