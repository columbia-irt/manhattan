#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libxml/parser.h>

#include "locmgr.h"
#include "pe.h"

/* TODO: hard code the structure of evaluation */
typedef struct eval {
	xmlChar *location;
	int bandwidth;
	int cost;
	xmlChar *interface;
	struct eval *next;
} eval_t;

typedef struct policy {
	int appID;
	struct eval eval_list;
	struct policy *next;
} policy_t;

#define EVAL_HEAD {		\
	.location = NULL,	\
	.bandwidth = -1,	\
	.cost = -1,		\
	.interface = NULL,	\
	.next = NULL,		\
}

#define POLICY_HEAD {		\
	.appID = -1,		\
	.eval_list = EVAL_HEAD,	\
	.next = NULL,		\
}

#define valid_str(x) (x ? (char *)x : "(null)")

static policy_t policy_list = POLICY_HEAD;
static volatile int need_evaluation = 0;

void trigger_policy_engine()
{
	need_evaluation = 1;
}

static void dump_policies()
{
	policy_t *policy = policy_list.next;
	eval_t *eval;
	log_info("================================================");
	log_info("|           Printing All Policies              |");
	log_info("================================================");
	while (policy != NULL) {
		log_info("Application ID: %d", policy->appID);
		eval = policy->eval_list.next;
		while (eval != NULL) {
			debug("location %s, bandwidth %d, cost %d -> interface %s",
				valid_str(eval->location),
				eval->bandwidth, eval->cost,
				valid_str(eval->interface));
			eval = eval->next;
		}
		log_info("================================================");
		policy = policy->next;
	}
}

static void parse_evaluation(xmlDocPtr doc, xmlNodePtr cur, eval_t *eval)
{
	xmlChar *tmp;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"location")) {
			eval->location = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"cost")) {
			tmp = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
			eval->cost = strtol((const char *)tmp, NULL, 10);
			xmlFree(tmp);
		} else if (!xmlStrcmp(cur->name,
				(const xmlChar *)"bandwidth")) {
			tmp = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
			eval->bandwidth = strtol((const char *)tmp, NULL, 10);
			xmlFree(tmp);
		} else if (!xmlStrcmp(cur->name,
				(const xmlChar *)"interface")) {
			eval->interface = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
		}

		cur = cur->next;
	}
}

static void parse_policy(xmlDocPtr doc, xmlNodePtr cur, policy_t *policy)
{
	eval_t *eval = &policy->eval_list;
	xmlChar *tmp;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"appID")) {
			tmp = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
			policy->appID = strtol((const char *)tmp, NULL, 10);
			xmlFree(tmp);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"eval")) {
			eval->next = malloc(sizeof(eval_t));
			eval = eval->next;
			eval->location = NULL;
			eval->bandwidth = -1;
			eval->cost = -1;
			eval->interface = NULL;
			eval->next = NULL;

			parse_evaluation(doc, cur, eval);
		}

		cur = cur->next;
	}
}

void parse_policies(const char *policy_path)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	policy_t *policy = &policy_list;
	eval_t head = EVAL_HEAD;

	doc = xmlParseFile(policy_path);
	check(doc, "Failed to parse policy file %s", policy_path);

	cur = xmlDocGetRootElement(doc);
	check(cur, "The policy file %s is empty", policy_path);
	check(xmlStrcmp(cur->name, (const xmlChar *)"UserPolicies") == 0,
		"Policy file of the wrong type, root node != UserPolicies");

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		//debug("%s", cur->name);
		if (!xmlStrcmp(cur->name, (const xmlChar *)"policy")) {
			//debug("new policy found");

			policy->next = malloc(sizeof(policy_t));
			policy = policy->next;
			policy->appID = -1;
			memcpy(&policy->eval_list, &head, sizeof(eval_t));
			policy->next = NULL;

			parse_policy(doc, cur, policy);
		}

		cur = cur->next;
	}

	dump_policies();

error:
	if (doc)
		xmlFreeDoc(doc);
	/* TODO: return parse failure */
}

static void evaluate()
{
	policy_t *policy = policy_list.next;
	eval_t *eval;
	char location[LOC_BUF_SIZE];

	get_location(location);
	debug("the current location is %s", location);

	while (policy) {
		log_info("evaluate for appID %d", policy->appID);

		eval = policy->eval_list.next;
		while (eval) {
			if (!xmlStrcmp(eval->location,
					(const xmlChar *)location)) {
				log_info("using interface %s",
						eval->interface);
				break;
			}
			eval = eval->next;
		}

		policy = policy->next;
	}
}

void * main_policy_engine(void *arg)
{
	while (1) {
		if (need_evaluation) {
			debug("policy engine triggered");
			evaluate();
			need_evaluation = 0;
		}
	}
}
