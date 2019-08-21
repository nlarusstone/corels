#include <stdio.h>
#include <iostream>
#include <set>

#include "queue.hh"
#include "run.hh"

#define BUFSZ 512

NullLogger* logger = nullptr;

int run_corels_begin(double c, char* vstring, int curiosity_policy,
                  int map_type, int ablation, int calculate_size, int nrules, int nlabels,
                  int nsamples, rule_t* rules, rule_t* labels, rule_t* meta, int freq, char* log_fname,
                  PermutationMap*& pmap, CacheTree*& tree, Queue*& queue, double& init,
                  std::set<std::string>& verbosity)
{
    verbosity.clear();

    const char *voptions = "rule|label|minor|samples|progress|loud";

    char *vopt = NULL;
    char *vcopy = m_strdup(vstring);
    char *vcopy_begin = vcopy;
    while ((vopt = m_strsep(&vcopy, ',')) != NULL) {
        if (!strstr(voptions, vopt)) {
            fprintf(stderr, "verbosity options must be one or more of (%s)\n", voptions);
            return -1;
        }
        verbosity.insert(vopt);
    }
    free(vcopy_begin);

    if (verbosity.count("loud")) {
        verbosity.insert("progress");
        verbosity.insert("label");
        verbosity.insert("rule");
        verbosity.insert("minor");
    }

#ifndef GMP
    if (verbosity.count("progress"))
        printf("**Not using GMP library**\n");
#endif

    if (verbosity.count("rule")) {
        printf("%d rules %d samples\n\n", nrules, nsamples);
        rule_print_all(rules, nrules, nsamples, verbosity.count("samples"));
        printf("\n\n");
    }

    if (verbosity.count("label")) {
        printf("Labels (%d) for %d samples\n\n", nlabels, nsamples);
        rule_print_all(labels, nlabels, nsamples, verbosity.count("samples"));
        printf("\n\n");
    }

    if (verbosity.count("minor") && meta) {
        printf("Minority bound for %d samples\n\n", nsamples);
        rule_print_all(meta, 1, nsamples, verbosity.count("samples"));
        printf("\n\n");
    }

    if (tree)
        delete tree;
    tree = nullptr;

    if (queue)
        delete queue;
    queue = nullptr;

    if (pmap)
        delete pmap;
    pmap = nullptr;

    int v = 0;
    if (verbosity.count("loud"))
        v = 1000;
    else if (verbosity.count("progress"))
        v = 1;

    if(!logger) {
        if(log_fname)
            logger = new Logger(c, nrules, v, log_fname, freq);
        else {
            logger = new PyLogger();
        }
    }
    logger->setVerbosity(v);

    init = timestamp();
    char run_type[BUFSZ];
    strcpy(run_type, "LEARNING RULE LIST via ");
    char const *type = "node";
    if (curiosity_policy == 1) {
        strcat(run_type, "CURIOUS");
        queue = new Queue(curious_cmp, run_type);
        type = "curious";
    } else if (curiosity_policy == 2) {
        strcat(run_type, "LOWER BOUND");
        queue = new Queue(lb_cmp, run_type);
    } else if (curiosity_policy == 3) {
        strcat(run_type, "OBJECTIVE");
        queue = new Queue(objective_cmp, run_type);
    } else if (curiosity_policy == 4) {
        strcat(run_type, "DFS");
        queue = new Queue(dfs_cmp, run_type);
    } else {
        strcat(run_type, "BFS");
        queue = new Queue(base_cmp, run_type);
    }

    if (map_type == 1) {
        strcat(run_type, " Prefix Map\n");
        PrefixPermutationMap* prefix_pmap = new PrefixPermutationMap;
        pmap = (PermutationMap*) prefix_pmap;
    } else if (map_type == 2) {
        strcat(run_type, " Captured Symmetry Map\n");
        CapturedPermutationMap* cap_pmap = new CapturedPermutationMap;
        pmap = (PermutationMap*) cap_pmap;
    } else {
        strcat(run_type, " No Permutation Map\n");
        NullPermutationMap* null_pmap = new NullPermutationMap;
        pmap = (PermutationMap*) null_pmap;
    }

    tree = new CacheTree(nsamples, nrules, c, rules, labels, meta, ablation, calculate_size, type);
    if (verbosity.count("progress"))
        printf("%s", run_type);

    bbound_begin(tree, queue);

    return 0;
}

int run_corels_loop(size_t max_num_nodes, PermutationMap* pmap, CacheTree* tree, Queue* queue)
{
    if((tree->num_nodes() < max_num_nodes) && !queue->empty()) {
        bbound_loop(tree, queue, pmap);
        return 0;
    }
    return -1;
}

double run_corels_end(int** rulelist, int* rulelist_size, int** classes, int early, int latex_out, rule_t* rules,
                      rule_t* labels, char* opt_fname, PermutationMap*& pmap, CacheTree*& tree, Queue*& queue,
                      double init, std::set<std::string>& verbosity)
{
    bbound_end(tree, queue, pmap, early);

    const tracking_vector<unsigned short, DataStruct::Tree>& r_list = tree->opt_rulelist();
    const tracking_vector<bool, DataStruct::Tree>& preds = tree->opt_predictions();

    double accuracy = 1.0 - tree->min_objective() + tree->c() * r_list.size();

    *rulelist = (int*)malloc(sizeof(int) * r_list.size());
    *classes = (int*)malloc(sizeof(int) * (1 + r_list.size()));
    *rulelist_size = r_list.size();
    for(size_t i = 0; i < r_list.size(); i++) {
        (*rulelist)[i] = r_list[i];
        (*classes)[i] = preds[i];
    }
    (*classes)[r_list.size()] = preds.back();

    if (verbosity.count("progress")) {
        printf("final num_nodes: %zu\n", tree->num_nodes());
        printf("final num_evaluated: %zu\n", tree->num_evaluated());
        printf("final min_objective: %1.5f\n", tree->min_objective());
        printf("final accuracy: %1.5f\n", accuracy);
        printf("final total time: %f\n", time_diff(init));
    }

    if(opt_fname) {
        print_final_rulelist(r_list, tree->opt_predictions(), latex_out, rules, labels, opt_fname);
        logger->dumpState();
        logger->closeFile();
    }

    // Exiting early skips cleanup
    if(!early) {    
        if (tree)
            delete tree;
        if (queue)
            delete queue;
        if (pmap)
            delete pmap;
    }

    tree = nullptr;
    queue = nullptr;
    pmap = nullptr;

    return accuracy;
}
