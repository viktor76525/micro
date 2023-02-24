#!/usr/bin/env python
"""
Usage: run script with space separated letters as arguments.
"""

from gzip import GzipFile
import json
import sys
from time import sleep

import numpy as np
from pyautogui import press, typewrite


def load_words_sample():
    # https://s3.amazonaws.com/wordsapi/wordsapi_sample.zip
    with open("wordsapi_sample.json", "r") as sample:
        dictionary = json.load(sample)
    words = []
    for word in dictionary.keys():
        if "definitions" in dictionary[word] and not any(bad_char in word for bad_char in [" ", "'", "-", "."]):
            words.append(word)
    return np.char.lower(np.array(words, dtype=str))


def load_words_all():
    # this is slow, just use the file decompressed
    gz_file = GzipFile("words.tar.gz")
    #gz_file = "definitions_words.txt"
    dictionary = np.char.lower(np.genfromtxt(gz_file, dtype=str, delimiter="\n"))
    return dictionary


letters = list(map(str.lower, sys.argv[1:]))
dictionary = load_words_all()

trimmed = np.copy(dictionary)
for letter in letters:
    trimmed = np.char.replace(trimmed, letter, "", count = 1)
answers = dictionary[np.where(trimmed == "")[0]]
longest_indices = np.argsort(1000 - np.char.str_len(answers))

# this is where the script starts typing answers
# add a sleep before the loop if loading is too fast to move cursor to slack
for word in answers[longest_indices]:
    print(word)
    typewrite(word)
    sleep(0.2)
    press("enter")
    # slack makes you wait on spammed messages, 0.2 + 0.15 seems fine
    sleep(0.15)
