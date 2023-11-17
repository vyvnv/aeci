#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>

using namespace std;

#define SHIFT 16

// возвращает индекс символа
int index(char c, vector<pair<char, unsigned int>> arr) {
    for (int i = 0; i < arr.size(); i++) {
        if (c == arr[i].first) {
            return i + 2;
        }
    }
    return -1;
}

// запись очередного бита сжатой информации
void output_bit(unsigned int bit, unsigned int* blen, unsigned char* insertbit, FILE* output) {
    (*insertbit) >>= 1;
    if (bit) (*insertbit) |= 0x80;
    (*blen)--;
    if ((*blen) == 0) {
        fputc((*insertbit), output);
        (*blen) = 8;
    }
}

//Записываем в файл последовательность битов
void sequence_of_bits(unsigned int bit, unsigned int* nextbit, unsigned int* blen, unsigned char* wbit, 
FILE* output) {
    output_bit(bit, blen, wbit, output);
    for (; *nextbit > 0; (*nextbit)--) {
        output_bit(!bit, blen, wbit, output);
    }
}

//функция побитового считывания из файла
int bitwiseInput(unsigned char* rbit, unsigned int* blen, FILE* input, unsigned int* remains) {
    if ((*blen) == 0) {
        (*rbit) = fgetc(input);
        if (feof(input)) {
            (*remains)++;
        }
        (*blen) = 8;
    }
    int t = (*rbit) & 1;
    (*rbit) >>= 1;
    (*blen)--;
    return t;
}

// функция кодирования
void EnCoder(const char* original = "original.txt", const char* cipher = "cipher.txt") {
    FILE* input;
    fopen_s(&input, original, "r");

    if (!input) {
        puts("ERROR: original text does not open\n");
        exit(1);
    }

    uint16_t* alfabet = new uint16_t[256];//массив букв из файла
    for (int i = 0; i < 256; i++) {//заполняем нулями
        alfabet[i] = 0;
    }

    unsigned char c = 0;
    while (!feof(input)) { //заполняем символами из файла
        c = fgetc(input);
        if (!feof(input)) {
            alfabet[c]++;
        }
    }
    fclose(input);

    vector<pair<char, unsigned int>> arr;//символ и его частота
    for (int i = 0; i < 256; i++) {
        if (alfabet[i] != 0) {
            arr.push_back(make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(arr.begin(), arr.end(),[](const pair<char, unsigned int>& l, const pair<char, unsigned int>& r) { 
                if (l.second != r.second) {
                    return l.second >= r.second;
                }
                return l.first < r.first;
        }); // отсортировавла алфавит


    uint16_t* segment = new uint16_t[arr.size() + 2];
    segment[0] = 0;
    segment[1] = 1;
    for (int i = 0; i < arr.size(); i++) {
        unsigned int b = arr[i].second;
        for (int j = 0; j < i; j++) {
            b += arr[j].second;
        }
        segment[i + 2] = b;
    }

    fopen_s(&input, original, "r");
    if (!input) {
        puts("ERROR: original text does not open\n");
        exit(1);
    }

    FILE* output;
    fopen_s(&output, cipher, "w");
    if (!output) {
        puts("ERROR: cipher text does not open\n");
        exit(1);
    }


    char countsym = arr.size();//записываем в файл счетчик символов
    fputc(countsym, output);

    // записываем в файл символы и их частоты
    int j = 0;
    for (int i = 0; i < 256; i++) {
        if (alfabet[i] != 0) {
            fputc(static_cast<char>(i), output);
            fwrite(reinterpret_cast<const char*>(&alfabet[i]), sizeof(uint16_t), 1, output);
        }
    }

    unsigned int left = 0;
    unsigned int right = ((static_cast<unsigned int>(1)<< SHIFT) - 1);
    //использую static_cast для явного преобразования типов
    unsigned int numerator = right - left + 1;
    unsigned int denominator = segment[arr.size() + 1];
    //делим отрезок на части
    unsigned int part_one = (right + 1) / 4;
    unsigned int half = part_one * 2;//середина
    unsigned int part_three = part_one * 3;

    unsigned int nextbit = 0;
    unsigned int blen = 8;
    unsigned char insertbit = 0;

    while (!feof(input)) {
        c = fgetc(input);
        if (!feof(input)) {
            j = index(c, arr);
            // меняем  границы отрезка под считанный символ
            right = left + segment[j] * numerator / denominator - 1;
            left = left + segment[j - 1] * numerator / denominator;
            //побитово записываем в файл
            for (;;) { 
                if (right < half) {
                    sequence_of_bits(0, &nextbit, &blen, &insertbit, output);
                }
                else if (left >= half) {
                    sequence_of_bits(1, &nextbit, &blen, &insertbit, output);
                    left -= half;
                    right -= half;
                }
                else if ((left >= part_one) && (right < part_three)) {
                    nextbit++;
                    left -= part_one;
                    right -= part_one;
                }
                else {
                    break;
                }

                left += left;
                right += right + 1;
            }
        }
        else {  //нужен для кодирования кода конца файла
            right = left + segment[1] * numerator / denominator - 1;
            left = left + segment[0] * numerator / denominator;

            for (;;) {
                if (right < half) {
                    sequence_of_bits(0, &nextbit, &blen, &insertbit, output);
                }
                else if (left >= half) {
                    sequence_of_bits(1, &nextbit, &blen, &insertbit, output);
                    left -= half;
                    right -= half;
                }
                else if ((left >= part_one) && (right < part_three)) {
                    nextbit++;
                    left -= part_one;
                    right -= part_one;
                }
                else break;

                left += left;
                right += right + 1;
            }

            nextbit += 1;
            if (left < part_one) {
                sequence_of_bits(0, &nextbit, &blen, &insertbit, output);
            }
            else {
                sequence_of_bits(1, &nextbit,&blen, &insertbit, output);
            }
            insertbit >>= blen;
            fputc(insertbit, output);
        }
        numerator = right - left + 1;
    }
    fclose(input);
    fclose(output);
}

//функция декодирования
void DeCoder(const char* cipher = "cipher.txt", const char* decipher = "decipher.txt") {
    unsigned int* alfabet = new unsigned int[256];
    for (int i = 0; i < 256; i++) {
        alfabet[i] = 0;
    }
    FILE* input;
    fopen_s(&input, cipher, "r");
    if (!input) {
        puts("ERROR: cipher text does not open\n");
        exit(1);
    }

    unsigned char inputcount = 0;
    unsigned int count = 0;
    inputcount = fgetc(input);//считываем первый элемент
    if (!feof(input)) {//это кол-во символов
        count = static_cast<unsigned int>(inputcount);//преобразовываем
    }

    unsigned char c = 0;

    // считываем символы и их частоты
    for (int i = 0; i < count; i++) {
        c = fgetc(input);
        if (!feof(input)) {
            fread(reinterpret_cast<char*>(&alfabet[c]), sizeof(uint16_t), 1, input);
        }
    }

    vector<pair<char, unsigned int>> arr;//символ и его частота
    for (int i = 0; i < 256; i++) {
        if (alfabet[i] != 0) {
            arr.push_back(make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(arr.begin(), arr.end(),[](const std::pair<char, unsigned int>& l,const std::pair<char, unsigned int>& r) { // сортирую
                if (l.second != r.second) {
                    return l.second >= r.second;
                }
                return l.first < r.first;
        });


    uint16_t* segment = new uint16_t[arr.size() + 2];
    segment[0] = 0;
    segment[1] = 1;
    for (int i = 0; i < arr.size(); i++) {
        unsigned int b = arr[i].second;
        for (int j = 0; j < i; j++) {
            b += arr[j].second;
        }
        segment[i + 2] = b;
    }

    FILE* output;
    fopen_s(&output, decipher, "w");
    if (!input) {
        puts("ERROR: original text does not open\n");
        exit(1);
    }

    unsigned int left = 0;
    unsigned int right = ((static_cast<unsigned int>(1) << SHIFT) - 1);
    unsigned int numerator;
    unsigned int denominator = segment[arr.size() + 1];

    unsigned int part_one = (right + 1) / 4;
    unsigned int half = part_one * 2;
    unsigned int part_three = part_one * 3;


    unsigned int blen = 0;
    unsigned char rbit = 0;
    unsigned int remains = 0;
    uint16_t value = 0;
    int k = 0;

    // побитово считываем первый байт и находим значение кода
    for (int i = 1; i <= 16; i++) {
        k = bitwiseInput(&rbit, &blen, input, &remains);
        value = 2 * value + k;
    }
    numerator = right - left + 1;
    for (;;) {  // теперь считываем символы для раскодирования
        unsigned int frequency = static_cast<unsigned int>(((static_cast<unsigned int>(value) - left + 1) * 
denominator - 1) / numerator);
        int j;

        // ищем символ на отрезках
        for (j = 1; segment[j] <= frequency; j++) {}
        right = left + segment[j] * numerator / denominator - 1;
        left = left + segment[j - 1] * numerator / denominator;

        for (;;) {
            if (right < half) {
            }
            else if (left >= half) {
                left -= half;
                right -= half;
                value -= half;
            }
            else if ((left >= part_one) && (right < part_three)) {
                left -= part_one;
                right -= part_one;
                value -= part_one;
            }
            else {
                break;
            }

            left += left;
            right += right + 1;
            k = 0;
            k = bitwiseInput(&rbit, &blen, input, &remains);
            value += value + k;
        }

        if (j == 1) {
            break;
        }

        //записываем в файл раскодированный символ
        fputc(arr[j - 2].first, output);
        numerator = right - left + 1;
    }

    fclose(input);
    fclose(output);
}

void Equal(const char* original = "original.txt", const char* decipher = "decipher.txt") {
    FILE* input;
    fopen_s(&input, original, "r");
    FILE* output;
    fopen_s(&output, decipher, "r");
    if (!input || !output) {
        puts("ERROR: texts does not open\n");
        exit(1);
    }
    char a = fgetc(input), b = fgetc(output);
    while ((!feof(input)) && (!feof(output))) {
        a = fgetc(input);
        b = fgetc(output);
        if ((a != b) || (!feof(input)) && (feof(output)) || (feof(input)) && (!feof(output))) {
            puts("\nTexts are not equal\n");
            exit(1);
        }
    }
    cout << "\nTexts are equal\n";
    fclose(input);
    fclose(output);
}

int main() {
    EnCoder();
    DeCoder();
    Equal();
}