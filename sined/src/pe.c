#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libxml/parser.h>

#include "locmgr.h"
#include "conmgr.h"
#include "netmgr.h"
#include "pe.h"

/* TODO: hard code the structure of evaluation */
typedef struct rule {
	int ruleID;
	xmlChar *location;
	int bandwidth;
	int cost;
	xmlChar *interface;
	struct rule *next;
} rule_t;

typedef struct policy {
	xmlChar *app_desc;
	struct rule rule_list;
	struct policy *next;
} policy_t;

#define RULE_HEAD {		\
	.location = NULL,	\
	.bandwidth = -1,	\
	.cost = -1,		\
	.interface = NULL,	\
	.next = NULL,		\
}

#define POLICY_HEAD {		\
	.app_desc = NULL,	\
	.rule_list = RULE_HEAD,	\
	.next = NULL,		\
}

#define valid_str(x) (x ? (char *)x : "(null)")

static policy_t policy_list = POLICY_HEAD;
static volatile int need_evaluation = 0;

//only one metux to protect all policy evaluation
static pthread_mutex_t policy_mutex = PTHREAD_MUTEX_INITIALIZER;

void trigger_policy_engine()
{
	need_evaluation = 1;
}

static void dump_policies()
{
	policy_t *policy;
	rule_t *rule;

	log_info("================================================");
	log_info("|           Printing All Policies              |");
	log_info("================================================");

	pthread_mutex_lock(&policy_mutex);

	for (policy = policy_list.next; policy; policy = policy->next) {
		log_info("Application Descriptor: %s", policy->app_desc);
		rule = policy->rule_list.next;
		while (rule != NULL) {
			debug("location %s, bandwidth %d, cost %d -> interface %s",
				valid_str(rule->location),
				rule->bandwidth, rule->cost,
				valid_str(rule->interface));
			rule = rule->next;
		}
		log_info("================================================");
	}

	pthread_mutex_unlock(&policy_mutex);
}

/* TODO: valgrind test */
static void free_policies()
{
	policy_t *policy;
	rule_t *rule;

	pthread_mutex_lock(&policy_mutex);

	policy = policy_list.next;
	while (policy != NULL) {
		if (policy->app_desc)
			xmlFree(policy->app_desc);

		rule = policy->rule_list.next;
		while (rule != NULL) {
			if (rule->location)
				xmlFree(rule->location);
			if (rule->interface)
				xmlFree(rule->interface);
			policy->rule_list.next = rule->next;
			free(rule);
			rule = policy->rule_list.next;
		}

		policy_list.next = policy->next;
		free(policy);
		policy = policy_list.next;
	}
	pthread_mutex_unlock(&policy_mutex);
}

/* not thread safe, must be called while holding policy_mutex */
static void parse_rules(xmlDocPtr doc, xmlNodePtr cur, rule_t *rule)
{
	xmlChar *tmp;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"location")) {
			rule->location = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"cost")) {
			tmp = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
			rule->cost = strtol((const char *)tmp, NULL, 10);
			xmlFree(tmp);
		} else if (!xmlStrcmp(cur->name,
				(const xmlChar *)"bandwidth")) {
			tmp = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
			rule->bandwidth = strtol((const char *)tmp, NULL, 10);
			xmlFree(tmp);
		} else if (!xmlStrcmp(cur->name,
				(const xmlChar *)"interface")) {
			rule->interface = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
		}

		cur = cur->next;
	}
}

/* not thread safe, must be called while holding policy_mutex */
static void parse_policy(xmlDocPtr doc, xmlNodePtr cur, policy_t *policy)
{
	rule_t *rule = &policy->rule_list;
	int ruleID = 0;
	//xmlChar *tmp;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"appDesc")) {
			policy->app_desc = xmlNodeListGetString(doc,
						cur->xmlChildrenNode, 1);
			//policy->appID = strtol((const char *)tmp, NULL, 10);
			//xmlFree(tmp);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"rule")) {
			rule->next = malloc(sizeof(rule_t));
			rule = rule->next;

			++ruleID;
			rule->ruleID = ruleID;
			rule->location = NULL;
			rule->bandwidth = -1;
			rule->cost = -1;
			rule->interface = NULL;
			rule->next = NULL;

			parse_rules(doc, cur, rule);
		}

		cur = cur->next;
	}
}

void parse_policies(const char *policy_path)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	policy_t *policy;
	rule_t head = RULE_HEAD;

	free_policies();

	pthread_mutex_lock(&policy_mutex);
	policy = &policy_list;

	doc = xmlParseFile(policy_path);
	check(doc, "Failed to parse policy file %s", policy_path);

	cur = xmlDocGetRootElement(doc);
	check(cur, "The policy file %s is empty", policy_path);
	check(xmlStrcmp(cur->name, (const xmlChar *)"UserPolicies") == 0,
		"Policy file of the wrong type, root node != UserPolicies");

	for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
		//debug("%s", cur->name);
		if (!xmlStrcmp(cur->name, (const xmlChar *)"policy")) {
			//debug("new policy found");

			policy->next = malloc(sizeof(policy_t));
			policy = policy->next;
			policy->app_desc = NULL;
			memcpy(&policy->rule_list, &head, sizeof(rule_t));
			policy->next = NULL;

			parse_policy(doc, cur, policy);
		}
	}

	pthread_mutex_unlock(&policy_mutex);

	dump_policies();
	need_evaluation = 1;

error:
	if (doc)
		xmlFreeDoc(doc);
	pthread_mutex_unlock(&policy_mutex);
	/* TODO: return parse failure */
}

/* return: 0 - rule not taken, 1 - rule activated */
static inline int evaluate_rule(struct connection *conn, const rule_t *rule,
				const char *location)
{
	if (!xmlStrcmp(rule->location, (const xmlChar *)location)
		&& netmgr_interface_on((const char *)rule->interface)) {
		log_info("try to apply rule %d (%s)",
				rule->ruleID, rule->interface);
		if (!netmgr_set_interface(conn->addr,
			(const char *)rule->interface)) {
			conn->active_rule = rule->ruleID;
			log_info("Rule %d activated for app %s",
				rule->ruleID, conn->app_desc);
			return 1;
		}
		log_info("continue checking other rules");
	}
	return 0;
}

/* not thread safe, need con_tbl_mutex to be locked */
void evaluate(struct connection *conn)
{
	const policy_t *policy;
	const rule_t *rule;
	char location[LOC_BUF_SIZE];

	log_info("evaluate for app %s", conn->app_desc);

	locmgr_get_location(location);
	debug("the current location is %s", location);

	pthread_mutex_lock(&policy_mutex);
	for (policy = policy_list.next; policy; policy = policy->next) {
		if (xmlStrcmp(policy->app_desc,
			(const xmlChar *)conn->app_desc))
			continue;

		debug("policy found for app %s", policy->app_desc);
		for (rule = policy->rule_list.next; rule; rule = rule->next) {
			debug("evaluating rule %d...", rule->ruleID);
			if (evaluate_rule(conn, rule, location))
				break;
			debug("rule %d not match", rule->ruleID);
		}
	}
	pthread_mutex_unlock(&policy_mutex);
}

static void evaluate_all()
{
	struct connection *conn;

	pthread_mutex_lock(&con_tbl_mutex);
	for (conn = con_tbl_head->next; conn != NULL; conn = conn->next)
		evaluate(conn);
	pthread_mutex_unlock(&con_tbl_mutex);
}

void * main_policy_engine(void *arg)
{
	while (1) {
		if (need_evaluation) {
			debug("policy engine triggered");
			evaluate_all();
			need_evaluation = 0;
		}
	}
}
