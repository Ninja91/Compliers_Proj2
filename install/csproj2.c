#include <stdio.h>
#include "config.h"
#include "system.h"
#include <string.h>
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "cgraph.h"
#include "hashtab.h"
#include "langhooks.h"
#include "tree-iterator.h"


typedef struct defs_node {
    char *var;
    struct defs_node* next;
} def_node;
 

typedef struct def_node use_node;
typedef struct def_node uninitialized_vars;

typedef struct func_ctxt {
    def_node * local_defs_list;
    use_node * local_uses_list;
    uninitialized_vars * local_uninitialized_vars;

    def_node * global_defs_list;
    use_node * global_uses_list;
    uninitialized_vars * global_uninitialized_vars;

    def_node * parm_defs_list;
    use_node * parm_uses_list;
    uninitialized_vars * parm_uninitialized_vars;

    char * func_name;
} func_ctxt;

typedef struct func_ctxt genr_ctxt;


// For use in DFS function calling
typedef struct func_ctxt_node {
    char *ctxt_name;
    func_ctxt * func_ctx;
    struct func_ctxt_node* next;
} func_ctxt_node;



int search(def_node * head, char * var){ 
    while(head != NULL) {
        if (strcmp(head->var, var) == 0){
            return 1;
        }
        head = head->next;
    }
    return 0;
}


def_node* apnd_node(def_node * head, char * var){
    /*if (head == NULL) {

        def_node * new_head = (def_node *) xmalloc(sizeof(def_node)); 
        new_head->next = NULL;
        head->var = xmalloc(sizeof(char)*(strlen(var)+1));
        strcpy(new_head->var, var);
        return new_head;
    }

    def_node * head1 = head;
    for(; head->next != NULL; head = head->next);*/

    def_node * new_head = (def_node *) xmalloc(sizeof(def_node));
    new_head->next = head;
    new_head->var = xmalloc(sizeof(char)*(strlen(var)+1));
    strcpy(new_head->var, var);
    /*
    head->next = (def_node *) xmalloc(sizeof(def_node));
    head = head->next;
    head->next = NULL;
    head->var = xmalloc(sizeof(char)*(strlen(var)+1));
    strcpy(head->var, var);*/
    
    return new_head;
}

def_node* search_apnd(def_node * head, char * var){
    if (search(head, var)){
        return head;
    }
    else 
        return apnd_node(head, var);
}

void print_list(def_node * head){
    printf("[");
    while(head != NULL){
        printf(" %s", head->var);
        head = head->next;
    }

    printf("]");
    printf("\n");
}

void print_ctxt(func_ctxt * ctxt) {
    printf("\n******* %s CTXT **********\n", ctxt->func_name);
    printf("               Local           \n");
    print_list(ctxt->local_defs_list);
    print_list(ctxt->local_uses_list);
    print_list(ctxt->local_uninitialized_vars);
    printf("               GLOBAL           \n");
    print_list(ctxt->global_defs_list);
    print_list(ctxt->global_uses_list);
    print_list(ctxt->global_uninitialized_vars);
    printf("               Parms           \n");
    print_list(ctxt->parm_defs_list);
}

void traverse(tree node, FILE * out_fp, func_ctxt * fn_ctx); //traverse header

def_node * merge_lists(def_node * list0, def_node * list1){
    def_node * head;
    for(head = list1; head != NULL; head = head->next)
        if(!search(list0, head->var))
            list0 = search_apnd(list0, head->var);

    return list0;
}

def_node * add_non_intersec_2_list(def_node * list0, def_node * list1, def_node * list2){
    def_node * head;
#if 0 
    print_list(list0);
#endif
    for(head = list2; head != NULL; head = head->next)
        if (!search(list1, head->var))
            list0 = search_apnd(list0, head->var); 
    return list0;
}

def_node * merge_intersec_2_lists(def_node * list0, def_node * list1, def_node * list2){
    def_node * head;
#if 0 
    print_list(list0);
#endif
    for(head = list1; head != NULL; head = head->next)
        if (search(list2, head->var))
            list0 = search_apnd(list0, head->var); 
    for(head = list2; head != NULL; head = head->next)
        if (search(list1, head->var))
            list0 = search_apnd(list0, head->var); 
    return list0;
}

void merge_uses_and_unintialized_vars(genr_ctxt * dest_ctxt,genr_ctxt * source_ctxt){
    dest_ctxt->local_uninitialized_vars = add_non_intersec_2_list(dest_ctxt->local_uninitialized_vars,  \
            dest_ctxt->local_defs_list, \
            source_ctxt->local_uninitialized_vars);
        //merge_lists(dest_ctxt->local_uninitialized_vars, source_ctxt->local_uninitialized_vars);
    dest_ctxt->global_uninitialized_vars = add_non_intersec_2_list(dest_ctxt->global_uninitialized_vars,  \
            dest_ctxt->global_defs_list, \
            source_ctxt->global_uninitialized_vars);
    dest_ctxt->parm_uninitialized_vars = add_non_intersec_2_list(dest_ctxt->parm_uninitialized_vars,  \
            dest_ctxt->parm_defs_list, \
            source_ctxt->parm_uninitialized_vars);
    
    dest_ctxt->local_uses_list = merge_lists(dest_ctxt->local_uses_list, source_ctxt->local_uses_list);
    dest_ctxt->global_uses_list = merge_lists(dest_ctxt->global_uses_list, source_ctxt->global_uses_list);
    dest_ctxt->parm_uses_list = merge_lists(dest_ctxt->parm_uses_list, source_ctxt->parm_uses_list);
}

void merge_defs_ctxts(genr_ctxt * dest_ctxt, genr_ctxt * source1_ctxt, genr_ctxt * source2_ctxt){
    dest_ctxt->local_defs_list = merge_intersec_2_lists(dest_ctxt->local_defs_list, source1_ctxt->local_defs_list, source2_ctxt->local_defs_list);
    dest_ctxt->global_defs_list = merge_intersec_2_lists(dest_ctxt->global_defs_list, source1_ctxt->global_defs_list, source2_ctxt->global_defs_list);
    dest_ctxt->parm_defs_list = merge_intersec_2_lists(dest_ctxt->parm_defs_list, source1_ctxt->parm_defs_list, source2_ctxt->parm_defs_list);
}

void traverse_if_else(tree ifel, FILE * out_fp, func_ctxt * fn_ctx) {
	if (COND_EXPR_THEN (ifel) && (IS_EMPTY_STMT (COND_EXPR_THEN (ifel)) || TREE_CODE (COND_EXPR_THEN (ifel)) == GOTO_EXPR)
	      && COND_EXPR_ELSE (ifel)
	      && (IS_EMPTY_STMT (COND_EXPR_ELSE (ifel)) || TREE_CODE (COND_EXPR_ELSE (ifel)) == GOTO_EXPR)) {  
			return;	// return  
		  }
	
    genr_ctxt then_ctxt = {NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL, NULL, "then"};
    genr_ctxt else_ctxt = {NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL, NULL, "else"};
	traverse(COND_EXPR_THEN(ifel), out_fp, &then_ctxt);// traverse the block below the if clause
	traverse(COND_EXPR_ELSE(ifel), out_fp, &else_ctxt);// traverse the block below the else clause

    merge_uses_and_unintialized_vars(fn_ctx, &then_ctxt);
    merge_uses_and_unintialized_vars(fn_ctx, &else_ctxt);

    merge_defs_ctxts(fn_ctx, &then_ctxt, &else_ctxt);
#if 0
    print_ctxt(&then_ctxt);
    print_ctxt(&else_ctxt);
#endif
}

void traverse_stmt_list(tree stmts, FILE * out_fp, func_ctxt * fn_ctx) {
	tree_stmt_iterator itr; //iterator for iterating over all statment in statment lists
	int i;
	
	for(i = 0, itr = tsi_start(stmts); !tsi_end_p(itr); tsi_next(&itr), i++) {
		tree stmt = tsi_stmt(itr); //getting a statement using iterator
		
		traverse(stmt, out_fp, fn_ctx);//traverse the statement
	}
}

// Main function which branched different kinds of statements
void traverse(tree node, FILE * out_fp, func_ctxt * func_ctx) {
	if (node == NULL) // if NULL then don't do anything
		return;
	
	switch(TREE_CODE(node)) {
		case CALL_EXPR: {
				int i,is_scanf = 0;
                char * fn_name;
				call_expr_arg_iterator iter;
				tree arg;
				
				tree function_node = TREE_OPERAND(CALL_EXPR_FN(node), 0);
                fn_name = IDENTIFIER_POINTER(DECL_NAME(function_node));

                func_ctxt callee_fn_ctxt = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fn_name};
                if (!DECL_BUILT_IN(function_node)){
                    traverse(DECL_SAVED_TREE(function_node), out_fp, &callee_fn_ctxt);
                    printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
                    print_ctxt(&callee_fn_ctxt);
                    printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
                }
                merge_defs_ctxts(func_ctx, &callee_fn_ctxt, &callee_fn_ctxt);
				
                if (fn_name){
					fprintf(out_fp, "\n\n\n ***** func_name: %s : %s ", 
					fn_name, 
					DECL_BUILT_IN(function_node) ? "builtin" : "no builtin");
                    
                    if (strcmp(fn_name, "scanf") == 0 && DECL_BUILT_IN(function_node)) {
                        is_scanf = 1;
                    }
                }
				
				i =0;
				FOR_EACH_CALL_EXPR_ARG (arg, iter, node)
				{
				    char temp[10];
				    sprintf (temp, "arg %d", i);
                  
                    //TODO: take care of executing this only for scanf function
                    if (is_scanf){
                    if (TREE_CODE(arg) == ADDR_EXPR && TREE_CODE(TREE_OPERAND(arg, 0)) == VAR_DECL \
                            && (TREE_CODE(TREE_TYPE(TREE_OPERAND(arg, 0))) == INTEGER_TYPE ||   \
                                TREE_CODE(TREE_TYPE(TREE_OPERAND(arg, 0))) == REAL_TYPE)) {
                        char * var_name = IDENTIFIER_POINTER(DECL_NAME(TREE_OPERAND(arg, 0)));
                            func_ctx->local_defs_list = search_apnd(func_ctx->local_defs_list, var_name);
                    }
                    }
                    else
                        traverse (arg, out_fp, func_ctx);
				    i++;
				}
				// print_node(out_fp, "\n\n CALL_EXPR \n", node, 0);
				break;
			}
		
		case VAR_DECL: {
				int is_scalar_var = 0;
				switch(TREE_CODE(TREE_TYPE(node))){
					case INTEGER_TYPE:
					case REAL_TYPE:
						is_scalar_var = 1;
						break;
				}
			
                if(is_scalar_var){
				    char* var_type_name = IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(TREE_TYPE(node))));
				    char* var_name = IDENTIFIER_POINTER(DECL_NAME(node));
				    fprintf(out_fp, "\n\n\n\n\n#var : %s - %s -  %s  %s \n\n\n", \
                            var_type_name, IDENTIFIER_POINTER(DECL_NAME(node)), \
                            DECL_INITIAL(node) ? "initi" : "not_init", \
                            TREE_PUBLIC(node)? "public": "private");

                    if (TREE_PUBLIC(node)){ 
                        func_ctx->global_uses_list = search_apnd(func_ctx->global_uses_list, var_name);
                        if (!search(func_ctx->global_defs_list, var_name)) {
                            func_ctx->global_uninitialized_vars = search_apnd(func_ctx->global_uninitialized_vars, var_name);
                        }
                    }
                    else {
                        if(DECL_INITIAL(node))
                            func_ctx->local_defs_list = search_apnd(func_ctx->local_defs_list, var_name);
                    
                        func_ctx->local_uses_list = search_apnd(func_ctx->local_uses_list, var_name);
                        if (!search(func_ctx->local_defs_list, var_name)) 
                            func_ctx->local_uninitialized_vars = search_apnd(func_ctx->local_uninitialized_vars, var_name);
                    }
                }
            
				print_node(out_fp, "\n\n VAR_DECL \n", node, 0);
				break;
			}
						
		case DECL_EXPR: //variable declaration
			print_node(out_fp, "\n\n ****BIND_EXPR \n", node, 0);
			//traverse(TREE_OPERAND(node, 0), out_fp, func_ctx);
			break;
		case BIND_EXPR: 
			print_node(out_fp, "\n\n ****BIND_EXPR \n", node, 0);
		 	// print_node (out_fp, "vars", TREE_OPERAND (node, 0), 0);
	  	// 	print_node (out_fp, "body", TREE_OPERAND (node, 1), 0);
	  	// 	print_node (out_fp, "block", TREE_OPERAND (node, 2), 0);
			traverse(BIND_EXPR_BLOCK(node), out_fp, func_ctx);
			traverse(BIND_EXPR_BODY(node), out_fp, func_ctx); // get the body of the block(BIND_EXPR)
			break;
		case STATEMENT_LIST:
			if (!IS_EMPTY_STMT(node))
				traverse_stmt_list(node, out_fp, func_ctx);// resolving a statment list
			break;
		case COND_EXPR: 
			traverse_if_else(node, out_fp, func_ctx); /* resolving for if-else case 
								as well as condition expression of while*/
			break;
		case CASE_LABEL_EXPR: // case statement of a switch. Not to be counted. Body has to be counted.
		case LABEL_EXPR: // Associated with goto appearing when while and break/continue statements occur. Not to be counted.
		case PREDICT_EXPR: // Appears when continue is used. Not to be counted
			break;
		case SWITCH_EXPR: // Switch case
			traverse(SWITCH_BODY(node), out_fp, func_ctx); //get the switch case body and then traverse on thaat node
			break;
		
		// All binary tcc_binary operations
		case BIT_AND_EXPR:
		case BIT_IOR_EXPR:
		case BIT_XOR_EXPR:
		case CEIL_DIV_EXPR:
		case CEIL_MOD_EXPR:
		case EXACT_DIV_EXPR:
		case FLOOR_DIV_EXPR:
		case FLOOR_MOD_EXPR:
		case LROTATE_EXPR:
		case LSHIFT_EXPR:
		case MAX_EXPR:
		case MINUS_EXPR:
		case MIN_EXPR:
		case MULT_EXPR:
		case PLUS_EXPR:
		
		// tcc_comparisons
		case EQ_EXPR:
		case GE_EXPR:
		case GT_EXPR:
		case LE_EXPR:
		case LTGT_EXPR:
		case LT_EXPR:
		case NE_EXPR:
		case ORDERED_EXPR:
		case UNEQ_EXPR:
		case UNGE_EXPR:
		case UNGT_EXPR:
		case UNLE_EXPR:
		case UNLT_EXPR:
		case UNORDERED_EXPR:

		//tcc_expressions
		case POSTDECREMENT_EXPR:
		case POSTINCREMENT_EXPR:
		case PREDECREMENT_EXPR:
		case PREINCREMENT_EXPR:
		case TRUTH_ANDIF_EXPR:
		case TRUTH_AND_EXPR:
		case TRUTH_ORIF_EXPR:
		case TRUTH_OR_EXPR:
		case TRUTH_XOR_EXPR:

        case ARRAY_REF:
        case COMPONENT_REF:
			print_node(out_fp, "\n\n\n **** array_compnent-expr-class \n", node, 0);
			traverse(TREE_OPERAND(node, 0), out_fp, func_ctx);
			traverse(TREE_OPERAND(node, 1), out_fp, func_ctx);
            break;
        case MODIFY_EXPR:
			print_node(out_fp, "\n\n\n **** modify-expr-class \n", node, 0);           
			traverse(TREE_OPERAND(node, 1), out_fp, func_ctx);
            
			//int is_scalar_var = 0;
            tree node0 = TREE_OPERAND(node, 0);
            if (TREE_CODE(node0) == VAR_DECL) {
    			switch(TREE_CODE(TREE_TYPE(node0))) {
                    char * var_name;
		    		case INTEGER_TYPE:
			    	case REAL_TYPE:
                        var_name = IDENTIFIER_POINTER(DECL_NAME(node0));
				    	//is_scalar_var = 1;
                        if (TREE_PUBLIC(node0))
                            func_ctx->global_defs_list = search_apnd(func_ctx->global_defs_list, var_name);
                        else    
                            func_ctx->local_defs_list = search_apnd(func_ctx->local_defs_list, var_name);
    				break;  
                }
            }
            else if (TREE_CODE(node0) == INDIRECT_REF  && TREE_CODE(TREE_OPERAND(node0, 0)) == PARM_DECL){
                switch(TREE_CODE(TREE_TYPE(node0))) {
                    char * var_name;
                    case INTEGER_TYPE:
                    case REAL_TYPE:
                        node0 = TREE_OPERAND(node0, 0);
                        var_name = IDENTIFIER_POINTER(DECL_NAME(node0));
                        //is_scalar_var = 1;
                        func_ctx->parm_defs_list = search_apnd(func_ctx->parm_defs_list, var_name);
                    break;
                }

            }
			//if (!is_scalar_var) traverse(TREE_OPERAND(node, 0), out_fp, func_ctx);

            
			break;
        case INDIRECT_REF:
        case ADDR_EXPR:
        case TRUTH_NOT_EXPR:
        case VA_ARG_EXPR:
        case WITH_CLEANUP_EXPR:
        case IMAGPART_EXPR:
        case ABS_EXPR:
        case ADDR_SPACE_CONVERT_EXPR:
        case BIT_NOT_EXPR:
        case CONJ_EXPR:
        case CONVERT_EXPR:
        case FIXED_CONVERT_EXPR:
        case FIX_TRUNC_EXPR:
        case FLOAT_EXPR:
        case NEGATE_EXPR:
        case NON_LVALUE_EXPR:
        case NOP_EXPR:
		case RETURN_EXPR:
			traverse(TREE_OPERAND(node, 0), out_fp, func_ctx);
			break;
		default:
            print_node(out_fp, "\n\n\n *** default\n", node, 0);
	}
}

void traverse_function(tree fn, FILE * out_fp) {
	tree func_block = DECL_SAVED_TREE(fn); // getting the block following the function header
	//tree block_stmts = BIND_EXPR_BODY(func_block); // Converting this block(BIND_EXPR) into a liked list of statements

    fprintf(out_fp, "\n\n\n function: %s", IDENTIFIER_POINTER (DECL_NAME (fn)));
    func_ctxt * func_ctx = (func_ctxt *) xmalloc(sizeof(func_ctxt));
	func_ctx->func_name = IDENTIFIER_POINTER (DECL_NAME (fn));
	func_ctx->local_defs_list = NULL;
	func_ctx->local_uses_list = NULL;
    func_ctx->local_uninitialized_vars = NULL;
    func_ctx->global_defs_list = NULL;
    func_ctx->global_uses_list = NULL;
    func_ctx->global_uninitialized_vars = NULL;
    func_ctx->parm_defs_list = NULL;
	
    traverse(func_block, out_fp, func_ctx); //traverse the statement lists
    print_ctxt(func_ctx);
    //print_list(func_ctx);
}

// void cs502_proj2()
// {
  
//   printf("please implenment your analyzer of project2 here\n"); 

// }

void cs502_proj2(){
    struct varpool_node *glb_var_nodes;
    struct cgraph_node *func_nodes;
    tree fn;
    FILE *out_fp;
    int global_var_cnt = 0;
    int func_count = 0;

    out_fp = fopen("output.txt", "w"); // Opening the output file in write mode

	//varpool_nodes - Linked list of global variables
    for(glb_var_nodes = varpool_nodes; glb_var_nodes; glb_var_nodes = glb_var_nodes->next) {
            global_var_cnt++; // Counting global variables
    }
    
    for (func_nodes = cgraph_nodes; func_nodes; func_nodes = func_nodes->next) {
        func_count++; // Counting the number of functions cgraph_node. cgraph_nodes is the linked list of functions
        fn = func_nodes->decl; //getting function node containing func definition/declaration  in the AST
		traverse_function(fn, out_fp); //traverse a function
    }

	// dumping output in output file
	//fprintf(out_fp, "\n#functions : %d", func_count);
    //fprintf(out_fp, "\n#global vars: %d", global_var_cnt);
	//fprintf(out_fp, "\n#local vars: %d", local_var_cnt);
	//fprintf(out_fp, "\n#statements: %d", statements_cnt);
	//fprintf(out_fp, "\n");
	
    fclose(out_fp);
}

