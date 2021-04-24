import re
import nltk.data
from math import log
from os import listdir
from nltk.stem import WordNetLemmatizer
from rouge_score import rouge_scorer

BUSINESS = 'business'
ENTERTAINMENT = 'entertainment'
POLITICS = 'politics'
SPORT = 'sport'
TECH = 'tech'

STOP_WORDS = [line.strip() for line in open("stop_words")]
news_articles_path = 'BBC News Summary/News Articles/'
summaries_path = 'BBC News Summary/Summaries/'

tokenizer = nltk.data.load('tokenizers/punkt/english.pickle')


def tokenize_words(text):
    return list(map(str.lower, re.findall(r"[-\w']+", text)))


def tokenize_sentences(text):
    return tokenizer.tokenize(text.strip())


def eliminate_stopwords(words):
    result = []
    for word in words:
        if len(word) > 1 and word not in STOP_WORDS:
            result.append(word)
    return result


def lemmatize(words):
    wnl = WordNetLemmatizer()
    return [wnl.lemmatize(word) for word in words]


def train_classifier(training_set: dict):
    vocabulary = {}
    total_word_cnt = {BUSINESS: 0, ENTERTAINMENT: 0, SPORT: 0, POLITICS: 0, TECH: 0}
    for key in training_set:
        for doc in training_set[key]:
            for word in doc:
                if word in vocabulary:
                    vocabulary[word][key] += 1
                else:
                    vocabulary[word] = {BUSINESS: 0, ENTERTAINMENT: 0, SPORT: 0, POLITICS: 0, TECH: 0}
                    vocabulary[word][key] += 1
                total_word_cnt[key] += 1
    return vocabulary, total_word_cnt


def train_summarizer(training_set: dict, summaries: dict, elim_stopwords: bool, lemmatization: bool, bigrams: bool):
    vocabulary = {}
    total_word_cnt = [0, 0]
    if not bigrams:
        for key in training_set:
            for doc, summary in zip(training_set[key], summaries[key]):
                for sentence in doc:
                    words = tokenize_words(sentence)
                    if elim_stopwords:
                        words = eliminate_stopwords(words)
                    if (lemmatization):
                        words = lemmatize(words)
                    if sentence in summary:
                        for word in words:
                            if word in vocabulary:
                                vocabulary[word][0] += 1
                            else:
                                vocabulary[word] = [1, 0]
                            total_word_cnt[0] += 1
                    else:
                        for word in words:
                            if word in vocabulary:
                                vocabulary[word][1] += 1
                            else:
                                vocabulary[word] = [0, 1]
                            total_word_cnt[1] += 1
    else:
        for key in training_set:
            for doc, summary in zip(training_set[key], summaries[key]):
                for sentence in doc:
                    words = tokenize_words(sentence)
                    if elim_stopwords:
                        words = eliminate_stopwords(words)
                    if (lemmatization):
                        words = lemmatize(words)
                    if sentence in summary:
                        if not words:
                            continue
                        myit = iter(words)
                        word1 = next(myit)
                        while True:
                            try:
                                word2 = next(myit)
                                word = word1 + ":" + word2
                                if word in vocabulary:
                                    vocabulary[word][0] += 1
                                else:
                                    vocabulary[word] = [1, 0]
                                total_word_cnt[0] += 1
                                word1 = word2
                            except StopIteration:
                                break
                    else:
                        if not words:
                            continue
                        myit = iter(words)
                        word1 = next(myit)
                        while True:
                            try:
                                word2 = next(myit)
                                word = word1 + ":" + word2
                                if word in vocabulary:
                                    vocabulary[word][1] += 1
                                else:
                                    vocabulary[word] = [0, 1]
                                total_word_cnt[1] += 1
                                word1 = word2
                            except StopIteration:
                                break
    return vocabulary, total_word_cnt


# Precit the class of the document given as argument
def predict_class(vocabulary: dict, document: list, word_cnt: dict, alpha=1):
    prob_sum = {BUSINESS: log(0.2), ENTERTAINMENT: log(0.2), SPORT: log(0.2), POLITICS: log(0.2), TECH: log(0.2)}
    for word in document:
        if word in vocabulary:
            for key in prob_sum:
                prob_sum[key] += log((vocabulary[word][key] + alpha) / (word_cnt[key] + len(vocabulary) * alpha))
        else:
            for key in prob_sum:
                prob_sum[key] += log(alpha / (word_cnt[key] + len(vocabulary) * alpha))

    doc_class = max(prob_sum, key=prob_sum.get)
    return doc_class, prob_sum[doc_class]


# Predict if a sentence should be in the summary
def predict_sentence_in_sum(vocabulary: dict, sentence: list, word_cnt, alpha=1, elim_stopwords=False, lemmatization=False, bigrams=False):
    prob_sum = [log(0.5), log(0.5)]
    words = tokenize_words(sentence)
    if elim_stopwords:
        words = eliminate_stopwords(words)
    if (lemmatization):
        words = lemmatize(words)
    if not bigrams:
        for word in words:
            if word in vocabulary:
                for i in range(2):
                    prob_sum[i] += log((vocabulary[word][i] + alpha) / (word_cnt[i] + len(vocabulary) * alpha))
            else:
                for i in range(2):
                    prob_sum[i] += log(alpha / (word_cnt[i] + len(vocabulary) * alpha))
    else:
        if words:
            myit = iter(words)
            word1 = next(myit)
            while True:
                try:
                    word2 = next(myit)
                    word = word1 + ":" + word2
                    if word in vocabulary:
                        for i in range(2):
                            prob_sum[i] += log((vocabulary[word][i] + alpha) / (word_cnt[i] + len(vocabulary) * alpha))
                    else:
                        for i in range(2):
                            prob_sum[i] += log(alpha / (word_cnt[i] + len(vocabulary) * alpha))
                    word1 = word2
                except StopIteration:
                    break
    if prob_sum[0] > prob_sum[1]:
        return "yes", prob_sum[0]
    else:
        return "no", prob_sum[1]


def evaluate_classification(test_set: dict, vocabulary: dict, word_cnt: dict):
    conf_matrix = {}
    for key in word_cnt:
        conf_matrix[key] = {BUSINESS: 0, ENTERTAINMENT: 0, SPORT: 0, POLITICS: 0, TECH: 0}
    tp = 0
    fp = 0

    for key in test_set:
        for doc in test_set[key]:
            doc_class, prob = predict_class(vocabulary, doc, word_cnt)
            conf_matrix[key][doc_class] += 1
            if doc_class == key:
                tp += 1
            else:
                fp += 1

    precision = tp / (tp + fp)
    recall = {}
    for key in test_set:
        recall[key] = conf_matrix[key][key] / sum(conf_matrix[key].values())
    return precision, recall, conf_matrix


def evaluate_summarization(test_set: dict, vocabulary: dict, word_cnt, ref_summaries: dict, elim_stopwords, lemmatization, bigrams):
    my_summaries = {}
    for key in test_set:
        my_summaries[key] = []
        for doc in test_set[key]:
            summary = []
            for sentence in doc:
                (result, prob) = predict_sentence_in_sum(vocabulary, sentence, word_cnt, 1, elim_stopwords, lemmatization, bigrams)
                if result == "yes":
                    summary.append(sentence)
            my_summaries[key].append(summary)
    if bigrams:
        scorer = rouge_scorer.RougeScorer(['rouge2'], use_stemmer=False)
    else:
        scorer = rouge_scorer.RougeScorer(['rouge1'], use_stemmer=False)
    avg_p = 0
    avg_r = 0
    avg_f = 0
    total = 0
    for key in test_set:
        for mysum, ref in zip(my_summaries[key], ref_summaries[key]):
            if bigrams:
                score = scorer.score(" ".join(ref), " ".join(mysum))['rouge2']
            else:
                score = scorer.score(" ".join(ref), " ".join(mysum))['rouge1']
            avg_p += score.precision
            avg_r += score.recall
            avg_f += score.fmeasure
            total += 1
    return avg_p/total, avg_r/total, avg_f/total


def test_classifier(elim_stopwords: bool, lemmatization: bool):
    doc_words_data = {}
    training_set = {}
    test_set = {}
    for doctype in listdir(news_articles_path):
        doc_words_data[doctype] = []
        for filename in listdir(news_articles_path + doctype):
            with open(news_articles_path + doctype + '/' + filename, 'r') as file:
                data = file.read()
                # tokenize
                all_words = tokenize_words(data)
                if elim_stopwords:
                    all_words = eliminate_stopwords(all_words)
                if lemmatization:
                    all_words = lemmatize(all_words)
            doc_words_data[doctype].append(all_words)

    # split dataset
    for key in doc_words_data.keys():
        training_set[key] = doc_words_data[key][:int(0.75 * len(doc_words_data[key]))]
        test_set[key] = doc_words_data[key][int(0.75 * len(doc_words_data[key])):]
    (vocabulary, word_cnt) = train_classifier(training_set)
    (precision, recall, cm) = evaluate_classification(test_set, vocabulary, word_cnt)
    header = "CLASSIFIER RESULTS "
    if lemmatization:
        header += 'LEMMATIZATION '
    if elim_stopwords:
        header += 'STOPWORDS ELIMINATION '
    print(header)
    print("Precision: ", precision * 100, "%")
    for key in recall:
        print("Recall for ", key, " is ", recall[key] * 100, "%")
    print(cm)
    print('\n')


def test_summarizer(elim_stopwords: bool, lemmatization: bool, bigrams:bool):
    docs_sentences = {}
    summaries = {}
    training_set = {}
    test_set = {}
    for doctype in listdir(news_articles_path):
        docs_sentences[doctype] = []
        for filename in listdir(news_articles_path + doctype):
            with open(news_articles_path + doctype + '/' + filename, 'r') as file:
                data = file.read()
                # tokenize
                sentences = tokenize_sentences(data)
                # remove title
                sentences[0] = sentences[0].splitlines()[-1]
            docs_sentences[doctype].append(sentences)

    for doctype in listdir(summaries_path):
        summaries[doctype] = []
        for filename in listdir(summaries_path + doctype):
            with open(summaries_path + doctype + '/' + filename, 'r') as file:
                text = file.read()
                # add space after each sentence, otherwise nltk tokenizer wont work
                text = re.sub(r"\.([A-Z])", r". \1", text)
                sentences = tokenize_sentences(text)
            summaries[doctype].append(sentences)
    # split dataset
    test_summaries = {}
    for key in docs_sentences.keys():
        training_set[key] = docs_sentences[key][:int(0.75 * len(docs_sentences[key]))]
        test_set[key] = docs_sentences[key][int(0.75 * len(docs_sentences[key])):]
        test_summaries[key] = summaries[key][int(0.75 * len(summaries[key])):]
    (vocabulary, word_cnt) = train_summarizer(training_set, summaries, elim_stopwords, lemmatization, bigrams)
    (precision, recall, f_score) = evaluate_summarization(test_set, vocabulary, word_cnt, test_summaries, elim_stopwords, lemmatization, bigrams)
    header = "SUMMARIZER RESULTS "
    if bigrams:
        header += "BIGRAMS "
    if lemmatization:
        header += 'LEMMATIZATION '
    if elim_stopwords:
        header += 'STOPWORDS ELIMINATION '
    print(header)
    print("Precision: ", precision * 100, "%")
    print("Recall: ", recall * 100, "%")
    print("F-score: ", f_score * 100, "%")
    print('\n')


# main va rula toate variantele de teste pe rand si va afisa rezultatele la consola
if __name__ == '__main__':
    test_classifier(False, False)
    test_classifier(True, False)
    test_classifier(True, True)
    for i in [False, True]:
        test_summarizer(False, False, i)
        test_summarizer(True, False, i)
        test_summarizer(True, True, i)
