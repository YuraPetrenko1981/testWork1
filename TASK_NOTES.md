для измерения пиковой памяти процесса использовался скрипт PowerShell

while ($true) {
>>     Clear-Host
>>     Get-Process companion -ErrorAction SilentlyContinue | Select-Object Name, @{Name="PeakMemory(MB)";Expression={[math]::Round($_.PeakWorkingSet64 / 1MB, 2)}}
>>     Start-Sleep -Seconds 1
>> }

До изменений
был запущен сам процесс, далее был загружен файл big.csv
получен результат
companion        1919,93
далее были поочерёдно выбраны все режимы графика
companion        1932,51

После изменений
companion         387,55


для измерения времени от выбора big.csv до готовности таблицы и графика использовался
std::chrono::steady_clock, t0 = Момент фактического QFile::open() (до его вызова), t1 = Qt впервые отрисовал viewport таблицы после того, как все строки и ширины колонок установлены.
измерить готовность графика невозможно т к для отрисовки графика требуются действия пользователя

результат измерения 
до изменений
utc_ms=1793694011860 dt_ms=5025.797 status=ok rows=200000 cols=37 file=C:/Users/Юра/Downloads/big.csv
после изменений
utc_ms=1793783926878 dt_ms=2959.133 status=ok rows=200000 cols=37 file=C:/Users/Юра/Downloads/big.csv



-----------------------------------------------------------------
создал структуру отображающую строковые данные из файла в числовые типы.
храню в векторе, т к вектор при вычислениях лучше всего ложится в кэши процессора.
модель таблицы обращается в эти исходные данные.
чтение файла вынес в отдельный поток

Как запустить тесты
1. Сборка тестов:
cmake --build build\simu --target gtests-companion --parallel
2. Запуск:
build\simu\companion\src\tests\gtests-companion.exe