# text: String functions in SQLite

The `sqlean-text` extension provides a rich set of functions for working with text.

Also provides Unicode-aware functions for changing text case (upper, lower, title), plus a custom nocase collation.

Many of the functions are Postgres-compatible (i.e. they have the same alias and logic as in PostgreSQL). It can be useful when migrating from SQLite to PostgreSQL or vice versa.

Regular expression functions are in the separate [regexp](regexp.md) extension.

[Substrings and slicing](#substrings-and-slicing) •
[Search and match](#search-and-match) •
[Split and join](#split-and-join) •
[Trim and pad](#trim-and-pad) •
[Change case](#change-case) •
[Other modifications](#other-modifications) •
[String properties](#string-properties) •
[Invalid UTF-8](#invalid-utf-8) •
[Installation and usage](#installation-and-usage)

## Substrings and slicing

### text_substring

```text
text_substring(str, start [,length])
```

Extracts a substring of `length` characters starting at the `start` position (1-based). By default, extracts all characters from `start` to the end of the string.

```sql
select text_substring('hello world', 7);
-- world

select text_substring('hello world', 7, 5);
-- world
```

Postgres-compatible (`substr`), but not aliased as `substr` to avoid conflicts with the built-in `substr` SQLite function.

### text_slice

```
text_slice(str, start [,end])
```

Extracts a substring from the `start` position inclusive to the `end` position non-inclusive (1-based). By default, `end` is the end of the string.

Both `start` and `end` can be negative, in which case they are counted from the end of the string toward the beginning of the string.

```sql
select text_slice('hello world', 7);
-- world

select text_slice('hello world', 7, 12);
-- world

select text_slice('hello world', -5);
-- world

select text_slice('hello world', -5, -2);
-- wor
```

### text_left

```text
text_left(str, length)
```

Extracts a substring of `length` characters from the beginning of the string. For negative `length`, extracts all but the last `|length|` characters.

```sql
select text_left('hello world', 5);
-- hello

select text_left('hello world', -6);
-- hello
```

Postgres-compatible, aliased as `left`.

### text_right

```text
text_right(str, length)
```

Extracts a substring of `length` characters from the end of the string. For negative `length`, extracts all but the first `|length|` characters.

```sql
select text_right('hello world', 5);
-- world

select text_right('hello world', -6);
-- world
```

Postgres-compatible, aliased as `right`.

## Search and match

### text_index

```text
text_index(str, other)
```

Returns the first index of the `other` substring in the original string.

```sql
select text_index('hello yellow', 'ello');
-- 2

select text_index('hello yellow', 'x');
-- 0
```

Postgres-compatible, aliased as `strpos`.

### text_last_index

```text
text_last_index(str, other)
```

Returns the last index of the `other` substring in the original string.

```sql
select text_last_index('hello yellow', 'ello');
-- 8

select text_last_index('hello yellow', 'x');
-- 0
```

### text_contains

```text
text_contains(str, other)
```

Checks if the string contains the `other` substring.

```sql
select text_contains('hello yellow', 'ello');
-- 1

select text_contains('hello yellow', 'x');
-- 0
```

### text_has_prefix

```text
text_has_prefix(str, other)
```

Checks if the string starts with the `other` substring.

```sql
select text_has_prefix('hello yellow', 'hello');
-- 1

select text_has_prefix('hello yellow', 'yellow');
-- 0
```

Postgres-compatible, aliased as `starts_with`.

### text_has_suffix

```text
text_has_suffix(str, other)
```

Checks if the string ends with the `other` substring.

```sql
select text_has_suffix('hello yellow', 'hello');
-- 0

select text_has_suffix('hello yellow', 'yellow');
-- 1
```

### text_count

```text
text_count(str, other)
```

Counts how many times the `other` substring is contained in the original string.

```sql
select text_count('hello yellow', 'ello');
-- 2

select text_count('hello yellow', 'x') = 0;
-- 0
```

### text_like

```text
text_like(pattern, str)
```

Reports whether a string matches a pattern using the LIKE syntax.

```sql
select text_like('cóm_ está_', 'CÓMO ESTÁS');
-- 1

select text_like('ça%', 'Ça roule');
-- 1
```

Not aliased as `like` to avoid conflicts with the built-in `like` SQLite function.

## Split and join

### text_split

```text
text_split(str, sep, n)
```

Splits a string by a separator and returns the n-th part (counting from one). When `n` is negative, returns the `|n|`th-from-last part.

```sql
select text_split('one|two|three', '|', 2);
-- two

select text_split('one|two|three', '|', -1);
-- three

select text_split('one|two|three', ';', 2);
-- (empty string)
```

Postgres-compatible, aliased as `split_part`.

### text_concat

```text
text_concat(str, ...)
```

Concatenates strings and returns the resulting string. Ignores nulls.

```sql
select text_concat('one', 'two', 'three');
-- onetwothree

select text_concat('one', null, 'three');
-- onethree
```

Postgres-compatible, aliased as `concat`.

### text_join

```text
text_join(sep, str, ...)
```

Joins strings using the separator and returns the resulting string. Ignores nulls.

```sql
select text_join('|', 'one', 'two');
-- one|two

select text_join('|', 'one', null, 'three');
-- one|three
```

Postgres-compatible, aliased as `concat_ws`.

### text_repeat

```text
text_repeat(str, count)
```

Concatenates the string to itself a given number of times and returns the resulting string.

```sql
select text_repeat('one', 3);
-- oneoneone
```

Postgres-compatible, aliased as `repeat`.

## Trim and pad

### text_ltrim

```text
text_ltrim(str [,chars])
```

Trims certain characters (spaces by default) from the beginning of the string.

```sql
select text_ltrim('  hello');
-- hello

select text_ltrim('273hello', '123456789');
-- hello
```

Postgres-compatible, aliased as `ltrim`.

### text_rtrim

```text
text_rtrim(str [,chars])
```

Trims certain characters (spaces by default) from the end of the string.

```sql
select text_rtrim('hello  ');
-- hello

select text_rtrim('hello273', '123456789');
-- hello
```

Postgres-compatible, aliased as `rtrim`.

### text_trim

```text
text_trim(str [,chars])
```

Trims certain characters (spaces by default) from the beginning and end of the string.

```sql
select text_trim('  hello  ');
-- hello

select text_trim('273hello273', '123456789');
-- hello
```

Postgres-compatible, aliased as `btrim`.

### text_lpad

```text
text_lpad(str, length [,fill])
```

Pads the string to the specified length by prepending certain characters (spaces by default).

```sql
select text_lpad('hello', 7);
--   hello

select text_lpad('hello', 7, '*');
-- **hello
```

Postgres-compatible, aliased as `lpad`.

ℹ️ PostgreSQL does not support unicode strings in `lpad`, while this function does.

### text_rpad

```text
text_rpad(str, length [,fill])
```

Pads the string to the specified length by appending certain characters (spaces by default).

```sql
select text_rpad('hello', 7);
-- hello

select text_rpad('hello', 7, '*');
-- hello**
```

Postgres-compatible, aliased as `rpad`.

ℹ️ PostgreSQL does not support unicode strings in `rpad`, while this function does.

## Change case

### text_upper

```text
text_upper(str)
```

Transforms a string to upper case.

```sql
select text_upper('cómo estás');
-- CÓMO ESTÁS
```

Not aliased as `upper` to avoid conflicts with the built-in `upper` SQLite function.

### text_lower

```text
text_lower(str)
```

Transforms a string to lower case.

```sql
select text_lower('CÓMO ESTÁS');
-- cómo estás
```

Not aliased as `lower` to avoid conflicts with the built-in `lower` SQLite function.

### text_title

```text
text_title(str)
```

Transforms a string to title case.

```sql
select text_title('cómo estás');
-- Cómo Estás
```

### text_nocase

The `text_nocase` collating sequence compares strings without regard to case.

```sql
select 1 where 'cómo estás' = 'CÓMO ESTÁS';
-- (null)

select 1 where 'cómo estás' = 'CÓMO ESTÁS' collate text_nocase;
-- 1
```

## Other modifications

### text_replace

```text
text_replace(str, old, new [,count])
```

Replaces `old` substrings with `new` substrings in the original string, but not more than `count` times. By default, replaces all `old` substrings.

```sql
select text_replace('hello', 'l', '*');
-- he**o

select text_replace('hello', 'l', '*', 1);
-- he*lo
```

Postgres-compatible (`replace`), but not aliased as `replace` to avoid conflicts with the built-in `replace` SQLite function.

### text_translate

```text
text_translate(str, from, to)
```

Replaces each string character that matches a character in the `from` set with the corresponding character in the `to` set. If `from` is longer than `to`, occurrences of the extra characters in `from` are deleted.

```sql
select text_translate('hello', 'ol', '01');
-- he110

select text_translate('hello', 'ol', '0');
-- he0
```

Postgres-compatible, aliased as `translate`.

ℹ️ PostgreSQL does not support unicode strings in `translate`, while this function does.

### text_reverse

```text
text_reverse(str)
```

Reverses the order of the characters in the string.

```sql
select text_reverse('hello');
-- olleh
```

Postgres-compatible, aliased as `reverse`.

ℹ️ PostgreSQL does not support unicode strings in `reverse`, while this function does.

## String properties

### text_length

```text
text_length(str)
```

Returns the number of characters in the string.

```sql
select text_length('𐌀𐌁𐌂');
-- 3
```

Postgres-compatible, aliased as `char_length` and `character_length`.

### text_size

```text
text_size(str)
```

Returns the number of bytes in the string.

```sql
select text_size('𐌀𐌁𐌂');
-- 12
```

Postgres-compatible, aliased as `octet_length`.

### text_bitsize

```text
text_bitsize(str)
```

Returns the number of bits in the string.

```sql
select text_bitsize('one');
-- 24
```

Postgres-compatible, aliased as `bit_length`.

## Invalid UTF-8

SQLite does not check that a text value is valid UTF-8, so a string may contain arbitrary bytes (for example, after `cast(x'ff' as text)`). Text functions treat such bytes in one of three ways.

Character functions decode each malformed byte sequence as U+FFFD, the Unicode replacement character, one per maximal ill-formed subsequence. The valid characters around it are left alone:

```sql
select text_length(cast(x'41ff42' as text));
-- 3

select hex(text_reverse(cast(x'41ff42' as text)));
-- 42EFBFBD41
```

This applies to `text_substring`, `text_slice`, `text_left`, `text_right`, `text_index`, `text_last_index`, `text_like`, `text_ltrim`, `text_rtrim`, `text_trim`, `text_lpad`, `text_rpad`, `text_translate`, `text_reverse` and `text_length`.

Byte functions do not decode anything, so invalid bytes pass through unchanged:

```sql
select hex(text_replace(cast(x'41ff42' as text), 'A', 'Z'));
-- 5AFF42
```

This applies to `text_contains`, `text_has_prefix`, `text_has_suffix`, `text_count`, `text_split`, `text_join`, `text_concat`, `text_repeat`, `text_replace`, `text_size` and `text_bitsize`.

Case functions (`text_upper`, `text_lower`, and `text_title`) return the input unchanged if it is not valid UTF-8, since there is nothing sensible to convert:

```sql
select hex(text_upper(cast(x'41ff42' as text)));
-- 41FF42
```

The `text_nocase` collation decodes like the character functions, so different malformed sequences can compare equal — they all become U+FFFD:

```sql
select 1 where cast(x'e4b8' as text) = cast(x'ff' as text) collate text_nocase;
-- 1
```

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./text
sqlite> select text_reverse('hello');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
