// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

bool IsContainNumber3 (const int& i) {
	string str = to_string(i);
	return count(str.begin(), str.end(), '3') >= 1;
}

int main() {
	int j = 0;
	for (int i = 1; i <= 1000; ++i) {
		j += IsContainNumber3(i);
	}
	cout << j << endl;
}
// Напишите ответ здесь: 271


// Закомитьте изменения и отправьте их в свой репозиторий.