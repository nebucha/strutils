#include <stdio.h>
#include <stdlib.h>
#include <unicode/ustring.h>
#include <unicode/usearch.h>

#define MAX_TEXT_LENGTH 102400
#define MAX_PATTERN_LENGTH 1024

int count_U8_bytes(UChar*, int32_t);
int text_search(const char*, const char*);
void print_result();
void print_result_as_byte();

// 結果格納用の連結リスト
typedef struct match_result {
    int index;
    int length;
    struct match_result *next;
} MatchResult;

MatchResult *matchResult = NULL;        // 文字数単位
MatchResult *matchResultAsByte = NULL;  // バイト数単位
MatchResult *current = NULL;
MatchResult *b_current = NULL;
int prefix = 0;
int b_prefix = 0;

/* 初期化 */
void init() {
    // 結果格納リストを初期化
    matchResult = (MatchResult*)malloc(sizeof(MatchResult));
    matchResult->index = -1;
    matchResult->length = 0;
    matchResult->next = NULL;

    matchResultAsByte = (MatchResult*)malloc(sizeof(MatchResult));
    matchResultAsByte->index = -1;
    matchResultAsByte->length = 0;
    matchResultAsByte->next = NULL;

    current = matchResult;
    b_current = matchResultAsByte;
}

/* 検索を実行 */
int text_search(const char *text, const char *pattern) 
{
    if (matchResult == NULL) {
        init();
    }

    int32_t length = -1;                    // 文字列のサイズ、NULL終端文字列の場合は-1
    UErrorCode errorCode = U_ZERO_ERROR;    // エラーコード
    const char *locale = uloc_getDefault(); // ロケール

    // 対象テキストのUStringの生成
    UChar utext[MAX_TEXT_LENGTH * 4];
    int32_t utextCapacity = MAX_TEXT_LENGTH * 4;
    int32_t utextLength;
    u_strFromUTF8(utext, utextCapacity, &utextLength, text, length, &errorCode);

    // パターンのUStringの生成
    UChar upattern[MAX_PATTERN_LENGTH * 4];
    int32_t upatternCapacity = MAX_PATTERN_LENGTH * 4;
    int32_t upatternLength;
    u_strFromUTF8(upattern, upatternCapacity, &upatternLength, pattern, length, &errorCode);

    // UStringSearchの生成
    UStringSearch *search = usearch_open(upattern, length, utext, length, locale, NULL, &errorCode);

    // UStringSearchの生成失敗
    if (U_FAILURE(errorCode)) {
        fprintf(stderr, "UStringSearch couldn't be opened.\n");
        return -1;
    }

    int pre_position = 0;
    int b_pos = 0;

    // 検索実行
    int position = usearch_first(search, &errorCode);
    while (U_SUCCESS(errorCode) && position != USEARCH_DONE) {
        int matchLength = usearch_getMatchedLength(search);

        // 結果を格納
        MatchResult *res = (MatchResult*)malloc(sizeof(MatchResult));
        res->index = prefix + position;
        res->length = matchLength;
        res->next = NULL;
        current->next = res;
        current = res;

        // byte単位のマッチ位置と文字列長を計算
        UChar tmp[MAX_TEXT_LENGTH * 4];
        u_strncpy(tmp, utext + pre_position, position - pre_position);
        pre_position = position;
        b_pos += count_U8_bytes(tmp, u_strlen(tmp));
        usearch_getMatchedText(search, tmp, MAX_TEXT_LENGTH * 4, &errorCode);
        int b_matchLength = count_U8_bytes(tmp, matchLength);
        // 結果を格納
        MatchResult *b_res = (MatchResult*)malloc(sizeof(MatchResult));
        b_res->index = b_prefix + b_pos;
        b_res->length = b_matchLength;
        b_res->next = NULL;
        b_current->next = b_res;
        b_current = b_res;

        // 次のマッチ位置へ進む
        position = usearch_next(search, &errorCode);
    }

    // パターンが見つからなかった
    if (U_FAILURE(errorCode)) {
        fprintf(stderr, "Couldn't detect the keyword.\n");
    }

    // プレフィックスを更新
    prefix += utextLength;
    b_prefix += count_U8_bytes(utext, utextLength);

    usearch_close(search);

    return 0;
}

/* 結果を文字数単位で出力 */
void print_result() {
    MatchResult *current = matchResult;
    while (current->next != NULL) {
        current = current->next;
        printf("Detected at %d, length is %d.\n", current->index, current->length);
    }
}

/* 結果をバイト数単位で出力 */
void print_result_as_byte() {
    MatchResult *current = matchResultAsByte;
    while (current->next != NULL) {
        current = current->next;
        printf("Detected at %d, length is %d.\n", current->index, current->length);
    }
}

/* UChar文字列をUTF-8文字列に変換した場合のバイト数を数える */
int count_U8_bytes(UChar *text, int32_t length) {
    int size = 0;
    int32_t pos = 0;
    UChar32 out;
    while (pos < length) {
        U16_NEXT(text, pos, length, out);
        size += U8_LENGTH(out);
    }
    return size;
}

/* 格納された結果をクリアする */
void clear_result() {
    MatchResult *a_current = matchResult;
    MatchResult *next;
    while ((next=a_current->next) != NULL) {
        free(a_current);
        a_current = next;
    }
    free(next);
    matchResult = NULL;

    a_current = matchResultAsByte;
    while ((next=a_current->next) != NULL) {
        free(a_current);
        a_current = next;
    }
    free(next);
    matchResultAsByte = NULL;
    current = NULL;
    b_current = NULL;
    prefix = 0;
    b_prefix = 0;
}

int main(void)
{
    const char *text = "あいうえおポプテピピックさしすせそポプテピピックたちつてとポプポプテピピックなにぬねの"; 
    // 1つ目と3つ目のポプテピピックはコードポイント数7
    // 2つ目のポプテピピックはコードポイント数11

    const char *pattern = "ポプテピピック";     // コードポイント数7

    init();
    text_search(text, pattern);
    printf("%d - %d\n", prefix, b_prefix);

    text_search(text, pattern);
    printf("%d - %d\n", prefix, b_prefix);

    print_result();
    printf("\n");
    print_result_as_byte();

    clear_result();
}