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
//end 1 byte alignment
#pragma pack()

static void process_buffer_with_magic_number(uint8_t* buffer, uint32_t buffer_size, uint32_t magic_number) {
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] ^= (magic_number >> ((i % 4) * 8)) & 0xFF;
    }
}

#define SURE_NEW(p) {if (p == nullptr) {std::cout << "Memory allocation failed" << std::endl; return false;}}

#define SURE_READ(is, cnt) do { \
    if (is) \
      std::cout << "all characters read successfully."<<std::endl; \
    else    \
      std::cout << "read "<< (cnt) <<" but error: only " << is.gcount() << " could be read"<<std::endl; \
} while(0)

/*
Load six buffers from the merged file, memory allocate in this function
@param merged_file_name: merged file name
@param encoder_param_buffer: encoder param buffer
@param encoder_bin_buffer: encoder bin buffer
@param decoder_param_buffer: decoder param buffer
@param decoder_bin_buffer: decoder bin buffer
@param joint_param_buffer: joint param buffer
@param joint_bin_buffer: joint bin buffer
if load successfully, return true, otherwise return false
*/
bool load_from_merged_file(
    const std::string& merged_file_name, 
    uint8_t** encoder_param_buffer, 
    uint8_t** encoder_bin_buffer,
    uint8_t** decoder_param_buffer,
    uint8_t** decoder_bin_buffer,
    uint8_t** joint_param_buffer,
    uint8_t** joint_bin_buffer) {
    
    ///set all point to nullptr
    *encoder_param_buffer = nullptr;
    *encoder_bin_buffer = nullptr;
    *decoder_param_buffer = nullptr;
    *decoder_bin_buffer = nullptr;
    *joint_param_buffer = nullptr;
    *joint_bin_buffer = nullptr;

    ifstream merged_file(merged_file_name.c_str(), ios::binary);
    if (!merged_file) {
        cout << "Failed to open file: " << merged_file_name << endl;
        return false;
    }
    uint32_t magic_number = 0;
    merged_file.read((char*)&magic_number, sizeof(magic_number));
    SURE_READ(merged_file, sizeof(magic_number));
    
    //read EP file header and data
    file_header fh;
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "EP", 2) == 0);
    *encoder_param_buffer = new uint8_t[fh.file_size];
    SURE_NEW(*encoder_param_buffer);
    merged_file.read((char*)*encoder_param_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*encoder_param_buffer, fh.file_size, magic_number);

    //read EB file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "EB", 2) == 0);
    *encoder_bin_buffer = new uint8_t[fh.file_size];
    SURE_NEW(*encoder_bin_buffer);
    merged_file.read((char*)*encoder_bin_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*encoder_bin_buffer, fh.file_size, magic_number);

    //read DP file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "DP", 2) == 0);
    *decoder_param_buffer = new uint8_t[fh.file_size];
    SURE_NEW(*decoder_param_buffer);
    merged_file.read((char*)*decoder_param_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*decoder_param_buffer, fh.file_size, magic_number);

    //read DB file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "DB", 2) == 0);
    *decoder_bin_buffer = new uint8_t[fh.file_size];
    SURE_NEW(*decoder_bin_buffer);
    merged_file.read((char*)*decoder_bin_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*decoder_bin_buffer, fh.file_size, magic_number);

    //read JP file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "JP", 2) == 0);
    *joint_param_buffer = new uint8_t[fh.file_size];
    SURE_NEW(*joint_param_buffer);
    merged_file.read((char*)*joint_param_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    ///magic number xor
    process_buffer_with_magic_number(*joint_param_buffer, fh.file_size, magic_number);

    //read JB file header and data
    merged_file.read((char*)&fh, sizeof(fh));
    SURE_READ(merged_file, sizeof(fh));
    assert(memcmp(fh.file_id, "JB", 2) == 0);
    *joint_bin_buffer = new uint8_t[fh.file_size];
    SURE_NEW(*joint_bin_buffer);
    merged_file.read((char*)*joint_bin_buffer, fh.file_size);
    SURE_READ(merged_file, fh.file_size);
    merged_file.close();
    ///magic number xor
    process_buffer_with_magic_number(*joint_bin_buffer, fh.file_size, magic_number);

    return true;
}

/// load dp file and load the merged file, then compare them
bool load_and_verify(const string& dp_file_name, const uint8_t* loaded_buffer) {
    ifstream dp_file(dp_file_name.c_str(), ios::binary);
    if (!dp_file) {
        cout << "Failed to open file: " << dp_file_name << endl;
        return false;
    }
    
    //Get file size
    dp_file.seekg(0, ios::end);
    uint32_t dp_file_size = dp_file.tellg();
    dp_file.seekg(0, ios::beg);
    
    char* dp_buf = new char[dp_file_size];


    dp_file.read(dp_buf, dp_file_size);
    
    
    if (memcmp(dp_buf, loaded_buffer, dp_file_size) != 0) {
        cout << "File content not match" << endl;
        return false;
    }
    
    dp_file.close();

    delete[] dp_buf;
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


    /// load all 
    uint8_t* encoder_param_buffer = nullptr;
    uint8_t* encoder_bin_buffer = nullptr;
    uint8_t* decoder_param_buffer = nullptr;
    uint8_t* decoder_bin_buffer = nullptr;
    uint8_t* joint_param_buffer = nullptr;
    uint8_t* joint_bin_buffer = nullptr;
    if (load_from_merged_file(output_file, &encoder_param_buffer, &encoder_bin_buffer,
        &decoder_param_buffer, &decoder_bin_buffer, &joint_param_buffer, &joint_bin_buffer)) {
        cout << "Load merged file successfully" << endl;
    } else {
        cout << "Load merged file failed" << endl;
    }

    //verify six files and buffer
    if (load_and_verify(file_name[0], encoder_param_buffer)) {
        cout << "Verify encoder param file successfully" << endl;
    } else {
        cout << "Verify encoder param file failed" << endl;
    }
    if (load_and_verify(file_name[1], encoder_bin_buffer)) {
        cout << "Verify encoder bin file successfully" << endl;
    } else {
        cout << "Verify encoder bin file failed" << endl;
    }
    if (load_and_verify(file_name[2], decoder_param_buffer)) {
        cout << "Verify decoder param file successfully" << endl;
    } else {
        cout << "Verify decoder param file failed" << endl;
    }
    if (load_and_verify(file_name[3], decoder_bin_buffer)) {
        cout << "Verify decoder bin file successfully" << endl;
    } else {
        cout << "Verify decoder bin file failed" << endl;
    }
    if (load_and_verify(file_name[4], joint_param_buffer)) {
        cout << "Verify joint param file successfully" << endl;
    } else {
        cout << "Verify joint param file failed" << endl;
    }
    if (load_and_verify(file_name[5], joint_bin_buffer)) {
        cout << "Verify joint bin file successfully" << endl;
    } else {
        cout << "Verify joint bin file failed" << endl;
    }
    
    ///free all buffers
    delete[] encoder_param_buffer;
    delete[] encoder_bin_buffer;
    delete[] decoder_param_buffer;
    delete[] decoder_bin_buffer;
    delete[] joint_param_buffer;
    delete[] joint_bin_buffer;

    return 0;
}