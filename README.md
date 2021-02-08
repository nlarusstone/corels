# CORELS
Certifiably Optimal RulE ListS

**CORELS is a custom discrete optimization
technique for building rule lists over a categorical feature space.**
Our algorithm provides the optimal solution, with a certificate of optimality.
By leveraging algorithmic bounds, efficient data structures,
and computational reuse, we achieve several orders of magnitude speedup in time
and a massive reduction of memory consumption.
Our approach produces optimal rule lists on practical
problems in seconds.
This framework is a novel alternative to CART and other decision tree methods.

* Elaine Angelino, Nicholas Larus-Stone, Daniel Alabi, Margo Seltzer, and Cynthia Rudin.
**Learning Certifiably Optimal Rule Lists for Categorical Data**.
[JMLR](http://www.jmlr.org/papers/volume18/17-716/17-716.pdf), 2018.

* Nicholas Larus-Stone, Elaine Angelino, Daniel Alabi, Margo Seltzer, Vassilios Kaxiras, Aditya Saligrama, Cynthia Rudin
**Systems Optimizations for Learning Certifiably Optimal Rule Lists**
[SysML](https://saligrama.io/files/sysml.pdf), 2018

* Nicholas Larus-Stone. **Learning Certifiably Optimal Rule Lists: A Case For Discrete Optimization in the 21st Century**.
[Senior thesis](https://dash.harvard.edu/handle/1/38811502), 2017.

* Elaine Angelino, Nicholas Larus-Stone, Daniel Alabi, Margo Seltzer, Cynthia Rudin
**Learning certifiably optimal rule lists for categorical data**
[KDD](https://www.kdd.org/kdd2017/papers/view/learning-certifiably-optimal-rule-lists-for-categorical-data), 2017

CORELS is a custom branch-and-bound algorithm for optimizing rule lists.

**Web UI can be found at:** https://corels.eecs.harvard.edu/

**R Package can be found at:** https://cran.r-project.org/web/packages/corels/index.html

**Python package can be found at:** https://pypi.org/project/corels/

## Overview

* [C/C++ dependencies](#cc-dependencies)
* [Sample command](#sample-command)
* [Usage](#usage)
* [Data format](#data-format)
* [Arguments](#arguments)
* [Example dataset](#example-dataset)
* [Example rule list](#example-rule-list)
* [Optimization algorithm and objective](#optimization-algorithm-and-objective)
* [Data structures](#data-structures)
* [Related work](#related-work)

### C/C++ dependencies

* [gmp](https://gmplib.org/) (GNU Multiple Precision Arithmetic Library)
* [mpfr](http://www.mpfr.org/) (GNU MPFR Library for multiple-precision floating-point computations; depends on gmp)
* [libmpc](http://www.multiprecision.org/) (GNU MPC for arbitrarily high precision and correct rounding; depends on gmp and mpfr)

e.g., [install libmpc](http://brewformulas.org/Libmpc) via [homewbrew](https://brew.sh/) (this will also install gmp and mpfr):

    brew install libmpc

### Sample command

Run the following from the `src/` directory.

    make
    ./corels -r 0.015 -c 2 -p 1 ../data/compas_train.out ../data/compas_train.label ../data/compas_train.minor

### Usage

    ./corels [-b] [-n max_num_nodes] [-r regularization] [-v verbosity] -c (1|2|3|4) -p (0|1|2)
             [-f logging_frequency] -a (1|2|3) [-s] [-L latex_out] data.out data.label [data.minor]

### Data format

For examples, see `compas.out` and `compas.label` in `data/`.  Also see `compas.minor` (optional). Note that our data parsing is fragile and errors will occur if the format is not followed exactly.

* The input data files must be space-delimited text.
* Each line contains `N + 1` fields, where `N` is the number of observations, and ends with `\n` (including the last line).
* In each line, the last `N` fields are `0`'s and `1`'s, and encode a bit vector;
the first field has the format `{text-description}`, where the text between the brackets provides a description of the bit vector.
* There can be no spaces in the text descriptions--words should be separated by dashes or underscores.

### Arguments

**[-b]** Breadth-first search (BFS). You must specify a search policy;
use exactly one of `(-b | -c)`.

**[-n max_num_nodes]** Maximum trie (cache) size.
Stop execution if the number of nodes in the trie exceeds this number.
Default value corresponds to `-n 100000`.

**[-r regularization]** Regularization parameter (optional).
The default value corresponds to `-r 0.01` and can be thought of as adding a
penalty equivalent to misclassifying 1% of data when increasing a rule list's
length by one association rule.

**[-v verbosity]** Verbosity.
Default value corresponds to `-v 0`.
If verbosity is at least `10`, then print machine info.
If verbosity is at least `1000`, then also print input antecedents and labels.

**-c (1|2|3|4)** Best-first search policy.
You must specify a search policy; use exactly one of `(-b | -c)`.
We include four different prioritization schemes:

* Use `-c 1` to prioritize by curiosity (see *Section 5.1 Custom scheduling policies* of our paper).
* Use `-c 2` to prioritize by the lower bound.
* Use `-c 3` to prioritize by the objective.
* Use `-c 4` for depth-first search.

**-p (0|1|2)** Symmetry-aware map (optional).

* Use `-p 0` for no symmetry-aware map (default).
* Use `-p 1` for the permutation map.
* Use `-p 2` for the captured vector map.

**[-f logging_frequency]** Logging frequency.  Default value corresponds to `-f 1000`.

**-a (0|1|2)** Exclude a specific algorithm optimization.
Default value corresponds to `-a 0`.

* Use `-a 0` to include the following optimizations.
* Use `-a 1` to exclude the minimum support bounds (see *Section 3.7 Lower bounds on antecedent support* of our paper).
* Use `-a 2` to exclude the lookahead bound (see *Lemma 2 in Section 3.4 Hierarchical objective lower bound*).

**[-s]** Calculate upper bound on remaining search space size (optional).
This adds a small overhead; the default behavior does not perform the calculation.
With `-s`, we dynamically and incrementally calculate `floor(log10(Gamma(Rc, Q)))`,
where `Gamma(Rc, Q)` is the upper bound (see *Theorem 7 in Section 3.6* of our paper).

**[-L latex_out]** Latex output.  Include this flag to generate a latex representation of the output rule list.

**data.out** File path to training data.  See **Data format**, above.

* Encode `M` antecedents as `M` space-delimited lines of text. Each line contains `N + 1` fields.
* The first field has the format `{antecedent-description}`,
where the text between the brackets describes the antecedent, e.g., `{hair-color:brown}`, or `{age:20-25}`.
* The remaining `N` fields indicate whether the antecedent is true or false for the `N` observations.

**data.label** File path to training labels.  See **Data format**, above.

* Encode labels as two space-delimited lines of text.
*The first line starts with a description of the negative class label, e.g., `{label=0}`;
the remaining `N` fields indicate which of the `N` observations have this label.
* The second line contains analogous (redundant) information for the positive class label.

**data.minor** File path to a bit vector to support use of the equivalent points bound (optional, see *Theorem 20 in Section 3.14* of our paper).

### Example dataset

The files in `data/` were generated from [ProPublica's COMPAS recidivism dataset](https://github.com/propublica/compas-analysis),
specifically, the file `compas-scores-two-years.csv`.

We include one training set (`N = 6489`) and one test set (`N = 721`) from a 10-fold cross-validation experiment.
There are four training data files:

* `compas_train.csv` : `7` categorical and integer-valued features and binary class labels extracted from `compas-scores-two-years.csv`

    sex (Male, Female), age, juvenile-felonies, juvenile-misdemeanors, juvenile-crimes,
    priors, current-charge-degree (Misdemeanor, Felony)

* `compas_train-binary.csv` : `14` equivalent binary features and class labels (included for convenience / use with other algorithms)

    sex:Male, age:18-20, age:21-22, age:23-25, age:26-45, age:>45
    juvenile-felonies:>0, juvenile-misdemeanors:>0, juvenile-crimes:>0
    priors:2-3, priors:=0, priors:=1, priors:>3, current-charge-degree:Misdemeanor

* `compas_train.out` : `155` mined antecedents (binary features and length-2 conjunctions of binary features with support in `[0.005, 0.995]`), e.g.,

    {sex:Female}, {age:18-20}, {sex:Male,current-charge-degree:Misdemeanor}, {age:26-45,juvenile-felonies:>0}

* `compas_train.labels` : class labels

    {recidivate-within-two-years:No}, {recidivate-within-two-years:Yes}

* `compas_train.minor` : bit vector used to support the equivalent points bound

The corresponding test data files are: `compas_test.csv`, `compas_test-binary.csv`, `compas_test.out`, and `compas_test.labels`.

### Example rule list

    if (age=23−25) and (priors=2−3) then predict yes
    else if (age = 18 − 20) then predict yes
    else if (sex = male) and (age = 21 − 22) then predict yes
    else if (priors > 3) then predict yes
    else predict no

This **rule list** predicts two-year recidivism for the [ProPublica two-year recidivism dataset](https://github.com/propublica/compas-analysis),
and was found by CORELS.
Its **prefix** corresponds to the first four rules and its **default rule** corresponds to the last rule.

### Optimization algorithm and objective

CORELS is a custom branch-and-bound algorithm that minimizes the following objective
defined for a rule list `d`, training data `x` and corresponding labels `y`:

    R(d, x, y) = misc(d, x, y) + c * length(d).

This objective is a regularized empirical risk that consists of a loss
and a regularization term that penalizes longer rule lists.

* The loss `misc(d, x, y)` measures `d`'s misclassification error.
* The regularization parameter `c >= 0` is a small constant.
* `length(d)` is the number of rules in `d`'s prefix.

Let `p` be `d`'s prefix.  The following objective lower bound drives
our branch-and-bound procedure:

    b(p, x, y) = misc(p, x, y) + c * length(d) <= R(d, x, y)

where `misc(p, x, y)` is the prefix misclassification error
(due to mistakes made by the prefix, but not the default rule).

### Data structures

* A **trie** (prefix tree) functions as a cache and supports incremental computation.
* A **priority queue** supports multiple best-first search policies, as well as both breadth-first and depth-first search.
* A **map** supports symmetry-aware pruning.

### Related work

CORELS builds directly on:

* Hongyu Yang, Cynthia Rudin, and Margo Seltzer.
**Scalable Bayesian Rule Lists**. [arXiv:1602.08610](https://arxiv.org/abs/1602.08610), 2016. [code](https://github.com/Hongyuy/sbrlmod)

* Benjamin Letham, Cynthia Rudin, Tyler McCormick and David Madigan.
**Interpretable Classifiers Using Rules and Bayesian Analysis: Building a Better Stroke Prediction Model**.
*The Annals of Applied Statistics*, 2015, Vol. 9, No. 3, 1350–1371. [pdf](https://users.cs.duke.edu/~cynthia/docs/LethamRuMcMa15.pdf) [code](https://users.cs.duke.edu/~cynthia/code/BRL_supplement_code.zip)

In particular, CORELS uses a library by Yang et al. for efficiently representing and operating on bit vectors.
See the files [src/rule.h](src/rule.h) and [src/rule.c](src/rule.c).
