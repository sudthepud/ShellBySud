#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

typedef struct histNode{
	char **arguments;
	int argCount;
	struct histNode *next;
	struct histNode *prev;	
}HISTNODE;

typedef struct varNode{
    char *name;
    char *value;
    struct varNode *next;
    struct varNode *prev;
}VARNODE;

int main(int argc, char* argv[]) {
	if (argc > 2) {
		exit(1);
	}
	int histSize = 5;
	HISTNODE *head = NULL;
	VARNODE *headVar = NULL;

	if (argc == 1) {
		
		while (true) {
			printf("wsh> ");
			char *currLine = NULL;
			size_t len = 0;
			if (getline(&currLine, &len, stdin) == -1) {
				if (feof(stdin)) {
					free(currLine);
				    //printf("\n");
				    break;
				} else {
				    return -1;
				}
			}
			if (currLine[0] == '\n') {
				free(currLine);
				continue;
			}

			currLine[strlen(currLine) - 1] = '\0';
			
			char space[10] = " ";
			char *currLineCopy1 = malloc(strlen(currLine) + 1);
			strcpy(currLineCopy1, currLine);
			char *currLineCopy2 = malloc(strlen(currLine) + 1);
			strcpy(currLineCopy2, currLine);
			char *command = strsep(&currLineCopy1, space);
			char *command2 = strsep(&currLineCopy2, space);
			char *track;
			int count = 1;
			while ((track = strsep(&currLineCopy1, space)) != NULL) {
				count++;
			}
			char **arguments = (char **)malloc(sizeof(char *)*(count + 1));
			int countMax = 1;
			arguments[0] = command;
			while (countMax < count) {
				arguments[countMax] = strsep(&currLineCopy2, space);
				countMax++;
			}

			for (int i = 0; i < count; i++) {
				if (arguments[i][0] == '$') {
			    	char *varName = arguments[i] + 1;
			
			   		if (getenv(varName) != NULL) {
			  			arguments[i] = getenv(varName);
			    	} else {
			    		bool found = false;
			    		VARNODE *currVar = headVar;
			    		while (currVar != NULL) {
			    			if (strcmp(currVar->name, varName) == 0) {
			    				arguments[i] = strdup(currVar->value);
			    				found = true;
			    				break;
			    			}
			    			currVar = currVar->next;
			    		}
			    		if (found == false) {
				    		for (int j = i; j < count - 1; ++j) {
				        		arguments[j] = arguments[j + 1];
				        	}
				        	count--;
				        	i--;
				        }
			    	}
				}
			}
			arguments[count] = NULL;
			if (count == 0) {
				//free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				free(currLine);
				free(command2);
				free(command);
				continue;
			}

			int echoFlag = 0;
			int newCount = 0;
			for (int i = 0; i < count; i++) {
				char *concatenatedArgument;
				if (echoFlag) { 
					while (i < count && strcmp(arguments[i], "|") != 0) {
			 			if (concatenatedArgument == NULL) {
			 				concatenatedArgument = strdup(arguments[i]);
			 			} else {
			 				strcat(concatenatedArgument, " ");
			 			    strcat(concatenatedArgument, arguments[i]);
			 			}
			 			i++;
			 		}
			 		echoFlag = 0;
			 		arguments[newCount++] = concatenatedArgument;
			 		concatenatedArgument = NULL;
			 		if (i < count) {
			 			arguments[newCount++] = arguments[i];
			 		}
			 	} else {
			 	    if (strcmp(arguments[i], "echo") == 0) {
			 	    	echoFlag = 1;
			 	    	arguments[newCount++] = arguments[i];
			 	    } else {
			 	        arguments[newCount++] = arguments[i];
			 	    }
				}
			}
			arguments[newCount] = NULL;
		
			count = newCount;
			if (strcmp(command, "history") == 0) {
				if (arguments[1] == NULL) {
					HISTNODE *curr = head;
					int histNum = 1;
					while ((curr != NULL) && (histNum <= histSize)) {
						printf("%i) ", histNum);
						histNum++;
						int printCount = 0;
						while (printCount < (curr->argCount) - 1) {
							printf("%s ", curr->arguments[printCount]);
							printCount++;
						}
						printf("%s\n", curr->arguments[printCount]);
						curr = curr->next;
					}
				}
				else if (strcmp(arguments[1], "set") != 0) {
					int argNumber;
					sscanf(arguments[1], "%d", &argNumber);
					if ((argNumber > histSize) || (argNumber < 0)) {

					} else {
						int currNode = 1;
						HISTNODE *curr = head;
						bool valid = true; 
						while (currNode < argNumber) {
							if (curr == NULL) {
								valid = false;
								break;
							}
							curr = curr->next;
							currNode++;
						}
						if (curr == NULL) {
							valid = false;
						}
						if (valid == true) {
							int pipeCount = 1;
							bool hasPipe = false;
							for(int i = 0; i < curr->argCount; i++) {
								if (strcmp(curr->arguments[i], "|") == 0) {
									hasPipe = true;
									pipeCount++;
								}
							}
							if (hasPipe) {
								int pipeArgsIndex = 0;
								char **pipeArgs[pipeCount];
								int currPipe = 0;
								for (int i = 0; i < curr->argCount; i++) {
									if (strcmp(curr->arguments[i], "|") == 0) {
										char **toAdd = (char **)(malloc(sizeof(char *) * (i - currPipe + 1)));
										for (int j = 0; j < (i - currPipe); j++) {
											toAdd[j] = curr->arguments[currPipe + j];
										}
										toAdd[i-currPipe] = NULL;
										currPipe = i + 1;
										pipeArgs[pipeArgsIndex] = toAdd;
										pipeArgsIndex++;
									}
								}
								char **toAdd = (char **)(malloc(sizeof(char *) * (curr->argCount - currPipe + 1)));
								for (int j = 0; j < (curr->argCount - currPipe); j++) {
									toAdd[j] = curr->arguments[currPipe + j];
								}
								toAdd[curr->argCount - currPipe] = NULL;
								pipeArgs[pipeArgsIndex] = toAdd;

								int pipePrev[2];
								int pipeCurr[2];

								pid_t pgid = getpid();

								for (int i =0; i < pipeCount; i++) {
									if (i < pipeCount - 1) {
										if (pipe(pipeCurr) < 0) {
											return -1;
										}
									}
									pid_t childPid = fork();
									if (childPid < 0) {
										return -1;
									} else if (childPid == 0) {
										setpgid(0, pgid);
										if (i > 0) {
											dup2(pipePrev[0], 0);
											close(pipePrev[0]);
											close(pipePrev[1]);
										}
										if (i < pipeCount - 1) {
											dup2(pipeCurr[1], 1);
											close(pipeCurr[0]);
											close(pipeCurr[1]);
										}
										if (execvp(pipeArgs[i][0], pipeArgs[i]) == -1) {
											perror("execvp");
											return -1;
										}
									} else {
										if (i > 0) {
											close(pipePrev[0]);
											close(pipePrev[1]);
										}
										if (i < pipeCount - 1) {
											pipePrev[0] = pipeCurr[0];
											pipePrev[1] = pipeCurr[1];
										}
									}
								}
								int status;
								while (waitpid(-pgid, &status, WUNTRACED) > 0) {
									wait(NULL);
								}
								for (int i = 0; i < pipeCount; i++) {
									free(pipeArgs[i]);
								}
							} else {
								pid_t childPid = fork();
								if (childPid < 0) {
									return -1;
								} else if (childPid == 0) {
									if (execvp(curr->arguments[0], curr->arguments) == -1) {
										perror("execvp");
										exit(-1);
									}
									exit(0);
								} else {
									wait(NULL);
								}
							}
						}
					}
				}
				else if (strcmp(arguments[1], "set") == 0) {
					int newSize;
					sscanf(arguments[2], "%d", &newSize);
					if (head == NULL) {
						
					} else if (newSize == 0) {
						HISTNODE *tempFree = head;
						while (tempFree != NULL) {
							HISTNODE *next = tempFree->next;
							for (int i = 0; i < tempFree->argCount + 1; i++) {
								free(tempFree->arguments[i]);
							}
							free(tempFree->arguments);
							free(tempFree);
							tempFree= next;
						}
						head = NULL;
					}
					else if (newSize < histSize) {
						HISTNODE *curr = head;
						int currNode = 1;
						while ((currNode <= histSize) && (curr != NULL)) {
							if (curr->next == NULL) {
								break;
							}
							curr = curr->next;
							currNode++;
						}
						while (currNode > newSize) {
							HISTNODE *toFree = curr;
							curr = curr->prev;
							for (int i = 0; i < toFree->argCount; i++) {
							    free(toFree->arguments[i]);
							}
							free(toFree->arguments);
							free(toFree);
							currNode--;
						}
						curr->next = NULL;
					}
					histSize = newSize;
				}
				//free(arguments[0]);
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				free(currLine);
				free(command2);
				continue;
			}

			if (strcmp(command, "local") == 0) {
			    char *varAssign = strdup(arguments[1]);
			    VARNODE *toAdd = (VARNODE *)(malloc(sizeof(VARNODE)));
			    toAdd->name = strsep(&varAssign, "=");
			    toAdd->value = strdup(strsep(&varAssign, "\0"));
			    strcat(toAdd->value, "\0");
			    VARNODE *currVar = headVar;
			    bool conti = false;
			    while (currVar != NULL) {
			        if (strcmp(toAdd->name, currVar->name) == 0) {
			            if (strcmp(toAdd->value, "") == 0) {
			                if (currVar == headVar) {
			                    headVar = headVar->next;
			                    free(currVar);
			                } else if (currVar->next == NULL) {
			                    currVar->prev->next = NULL;
			                    free(currVar);
			                } else {
			                    currVar->prev->next = currVar->next;
			                    currVar->next->prev = currVar->prev;
			                    free(currVar);
			                }
			            } else {
			                currVar->value = toAdd->value;
			                free(toAdd);
			            }
			            conti = true;
			            break;
			        }
			        currVar = currVar->next;
			    }
			    if (conti != true) {
			        if (headVar == NULL) {
			            headVar = toAdd;
			            toAdd->next = NULL;
			            toAdd->prev = NULL;
			        } else {
			            VARNODE *end = headVar;
			            while (end->next != NULL) {
			                end = end->next;
			            }
			            end->next = toAdd;
			            toAdd->prev = end;
			            toAdd->next = NULL;
			        }
			    }
			    free(arguments[0]);
			    free(arguments);
			    free(currLineCopy1);
			    free(currLineCopy2);
			    free(currLine);
			    free(command2);
			    continue;
			}

			if (strcmp(command, "vars") == 0) {
			    VARNODE *currVar = headVar;
			    while (currVar != NULL) {
			        printf("%s=%s\n", currVar->name, currVar->value);
			        currVar = currVar->next;
			    }
			    free(arguments[0]);
			    free(arguments);
			    free(currLineCopy1);
			    free(currLineCopy2);
			    free(currLine);
			    free(command2);
			    continue;
			}

			if (strcmp(command, "export") == 0) {
			    char *varAssign = strdup(arguments[1]);
			    VARNODE *toAdd = (VARNODE *)malloc(sizeof(VARNODE));
			    toAdd->name = strsep(&varAssign, "=");
			    toAdd->value = strsep(&varAssign, "");
			    setenv(toAdd->name, toAdd->value, 1);
			    free(toAdd);
			    free(arguments[0]);
			    free(arguments);
			    free(currLineCopy1);
			    free(currLineCopy2);
			    free(currLine);
			    free(command2);
			    continue;
			}

			if (strcmp(command, "exit") == 0) {
				if (count > 1) {
					return -1;
				}
				free(arguments[0]);
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				free(currLine);
				free(command2);
				break;
			}

			if (strcmp(command, "cd") == 0) {
				if (count != 2) {
					return -1;
				} else {
					if (chdir(arguments[1]) != 0) {
						return -1;
					}
				}
				free(arguments[0]);
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				free(currLine);
				free(command2);
				continue;
			}

			if (histSize > 0) {
				
				HISTNODE *newHead = (HISTNODE *)(malloc(sizeof(HISTNODE)));
				newHead->arguments = malloc((count + 1) * sizeof(char*));
				newHead->arguments[0] = strdup(command);
				for (int i = 1; i < count; i++) {
					newHead->arguments[i] = strdup(arguments[i]);
				}
				newHead->arguments[count] = NULL; 
				newHead->argCount = count;
				newHead->prev = NULL;
				newHead->next = NULL;
				if (head == NULL) {
					head = newHead;
				} else {
					newHead->next = head;
					head->prev = newHead;
					head = newHead;
				}
				HISTNODE *curr = head;
				int currCount = 1;
				while (curr->next != NULL) {
					curr = curr->next;
					currCount++;
				}
				if (currCount < histSize) {
					
				} else {
					while (currCount > histSize) {
						HISTNODE *toFree = curr;
						curr = curr->prev;
						for (int i = 0; i < toFree->argCount; i++) {
						    free(toFree->arguments[i]);
						}
						free(toFree->arguments);
						free(toFree);
						currCount--;
					}
					curr->next = NULL;
				}
			}

			int pipeCount = 1;
			bool hasPipe = false;
			for (int i = 0; i < count; i++) {
				if (strcmp(arguments[i], "|") == 0) {
					hasPipe = true;
					pipeCount++;
				}
			}
			if (hasPipe) {
				int pipeArgsIndex = 0;
				char **pipeArgs[pipeCount];
				int currPipe = 0;
				for (int i = 0; i < count; i++) {
					if (strcmp(arguments[i], "|") == 0) {
						char **toAdd = (char **)(malloc(sizeof(char *) * (i - currPipe + 1)));
						for (int j = 0; j < (i - currPipe); j++) {
							toAdd[j] = arguments[currPipe + j];
						}
						toAdd[i-currPipe] = NULL;
						currPipe = i + 1;
						pipeArgs[pipeArgsIndex] = toAdd;
						pipeArgsIndex++;
					}
				}
				char **toAdd = (char **)(malloc(sizeof(char *) * (count - currPipe + 1)));
				for (int j = 0; j < (count - currPipe); j++) {
					toAdd[j] = arguments[currPipe + j];
				}
				toAdd[count - currPipe] = NULL;
				pipeArgs[pipeArgsIndex] = toAdd;	


				int pipePrev[2];
				int pipeCurr[2];

				pid_t pgid = getpid();
				
				for (int i = 0; i < pipeCount; i++) {
					if (i < pipeCount - 1) {
						if (pipe(pipeCurr) < 0) {
							return -1;
						}
					}
					pid_t childPid = fork();
					if (childPid < 0) {
						return -1;
					} else if (childPid == 0) {
						setpgid(0, pgid);
						if (i > 0) {
							dup2(pipePrev[0], 0);
							close(pipePrev[0]);
							close(pipePrev[1]);
						}
						if (i < pipeCount - 1) {
							dup2(pipeCurr[1], 1);
							close(pipeCurr[0]);
							close(pipeCurr[1]);
						}
						if (execvp(pipeArgs[i][0], pipeArgs[i]) == -1) {
							perror("execvp");
							exit(-1);
						}
					} else {
						if (i > 0) {
							close(pipePrev[0]);
							close(pipePrev[1]);
						}
						if (i < pipeCount - 1) {
							pipePrev[0] = pipeCurr[0];
							pipePrev[1] = pipeCurr[1];
						}
					}	
				}
				
				int status;
				while (waitpid(-pgid, &status, WUNTRACED) > 0) {
					wait(NULL);
				}

				for (int i = 0; i < pipeCount; i++) {
					free(pipeArgs[i]);
				}
							
			} else {		
				pid_t childPid = fork();
				if (childPid < 0) {
					return -1;
				} else if (childPid == 0) {
					if (execvp(arguments[0], arguments) == -1) {
						perror("execvp");
						exit(-1);
					}
					exit(0);
				} else {
					wait(NULL);
				}
			}
			//free(arguments[0]);
			free(arguments);
			free(currLineCopy1);
			free(currLineCopy2);
			free(currLine);
			free(command2);
		}
	}
	else if (argc == 2) {
		FILE *batchFile;
		if ((batchFile = fopen(argv[1], "r")) == NULL) {
			exit(-1);
		}
		while (true) {
			char *currLine = NULL;
			size_t len = 0;
			if (getline(&currLine, &len, batchFile) == -1) {
				usleep(500);
				if (feof(batchFile)) {
			    	free(currLine);
			        //printf("\n");
			        break;
			    } else {
			       	return -1;
			    }
			}
			if (currLine[0] == '\n') {
				continue;
			}

			currLine[strlen(currLine) - 1] = '\0';
			
			char space[10] = " ";
			char *currLineCopy1 = malloc(strlen(currLine) + 1);
			strcpy(currLineCopy1, currLine);
			char *currLineCopy2 = malloc(strlen(currLine) + 1);
			strcpy(currLineCopy2, currLine);
			char *command = strsep(&currLineCopy1, space);
			char *command2 = strsep(&currLineCopy2, space);
			free(currLine);
			char *track;
			int count = 1;
			while ((track = strsep(&currLineCopy1, space)) != NULL) {
				count++;
			}
			char **arguments = (char **)malloc(sizeof(char *)*(count + 1));
			int countMax = 1;
			arguments[0] = command;
			while (countMax < count) {
				arguments[countMax] = strsep(&currLineCopy2, space);
				countMax++;
			}

			for (int i = 0; i < count; i++) {
				if (arguments[i][0] == '$') {
			    	char *varName = arguments[i] + 1;
			
			   		if (getenv(varName) != NULL) {
			  			arguments[i] = getenv(varName);
			    	} else {
			    		bool found = false;
			    		VARNODE *currVar = headVar;
			    		while (currVar != NULL) {
			    			if (strcmp(currVar->name, varName) == 0) {
			    				arguments[i] = strdup(currVar->value);
			    				found = true;
			    				break;
			    			}
			    			currVar = currVar->next;
			    		}
			    		if (found == false) {
				    		for (int j = i; j < count - 1; ++j) {
				        		arguments[j] = arguments[j + 1];
				        	}
				        	count--;
				        	i--;
				        }
			    	}
				}
			}
			arguments[count] = NULL;
			if (count == 0) {
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				free(currLine);
				free(command2);
				free(command);
				continue;
			}

			int echoFlag = 0;
			int newCount = 0;
			for (int i = 0; i < count; i++) {
				char *concatenatedArgument;
				if (echoFlag) { 
					while (i < count && strcmp(arguments[i], "|") != 0) {
			 			if (concatenatedArgument == NULL) {
			 				concatenatedArgument = strdup(arguments[i]);
			 			} else {
			 				strcat(concatenatedArgument, " ");
			 			    strcat(concatenatedArgument, arguments[i]);
			 			}
			 			i++;
			 		}
			 		echoFlag = 0;
			 		arguments[newCount++] = concatenatedArgument;
			 		concatenatedArgument = NULL;
			 		if (i < count) {
			 			arguments[newCount++] = arguments[i];
			 		}
			 	} else {
			 	    if (strcmp(arguments[i], "echo") == 0) {
			 	    	echoFlag = 1;
			 	    	arguments[newCount++] = arguments[i];
			 	    } else {
			 	        arguments[newCount++] = arguments[i];
			 	    }
				}
			}
			arguments[newCount] = NULL;
			count = newCount;

			if (strcmp(arguments[0], "history") == 0) {
				if (arguments[1] == NULL) {
					HISTNODE *curr = head;
					int histNum = 1;
					while ((curr != NULL) && (histNum <= histSize)) {
						printf("%i) ", histNum);
						histNum++;
						int printCount = 0;
						while (printCount < (curr->argCount) - 1) {
							printf("%s ", curr->arguments[printCount]);
							printCount++;
						}
						printf("%s\n", curr->arguments[printCount]);
						curr = curr->next;
					}
				}
				else if (strcmp(arguments[1], "set") != 0) {
					int argNumber;
					sscanf(arguments[1], "%d", &argNumber);
					if ((argNumber > histSize) || (argNumber < 0)) {

					} else {
						int currNode = 1;
						HISTNODE *curr = head;
						bool valid = true; 
						while (currNode < argNumber) {
							if (curr == NULL) {
								valid = false;
								break;
							}
							curr = curr->next;
							currNode++;
						}
						if (curr == NULL) {
							valid = false;
						}
						if (valid == true) {
							int pipeCount = 1;
							bool hasPipe = false;
							for(int i = 0; i < curr->argCount; i++) {
								if (strcmp(curr->arguments[i], "|") == 0) {
									hasPipe = true;
									pipeCount++;
								}
							}
							if (hasPipe) {
								int pipeArgsIndex = 0;
								char **pipeArgs[pipeCount];
								int currPipe = 0;
								for (int i = 0; i < curr->argCount; i++) {
									if (strcmp(curr->arguments[i], "|") == 0) {
										char **toAdd = (char **)(malloc(sizeof(char *) * (i - currPipe + 1)));
										for (int j = 0; j < (i - currPipe); j++) {
											toAdd[j] = curr->arguments[currPipe + j];
										}
										toAdd[i-currPipe] = NULL;
										currPipe = i + 1;
										pipeArgs[pipeArgsIndex] = toAdd;
										pipeArgsIndex++;
									}
								}
								char **toAdd = (char **)(malloc(sizeof(char *) * (curr->argCount - currPipe + 1)));
								for (int j = 0; j < (curr->argCount - currPipe); j++) {
									toAdd[j] = curr->arguments[currPipe + j];
								}
								toAdd[curr->argCount - currPipe] = NULL;
								pipeArgs[pipeArgsIndex] = toAdd;

								int pipePrev[2];
								int pipeCurr[2];

								pid_t pgid = getpid();

								for (int i =0; i < pipeCount; i++) {
									if (i < pipeCount - 1) {
										if (pipe(pipeCurr) < 0) {
											return -1;
										}
									}
									pid_t childPid = fork();
									if (childPid < 0) {
										return -1;
									} else if (childPid == 0) {
										setpgid(0, pgid);
										if (i > 0) {
											dup2(pipePrev[0], 0);
											close(pipePrev[0]);
											close(pipePrev[1]);
										}
										if (i < pipeCount - 1) {
											dup2(pipeCurr[1], 1);
											close(pipeCurr[0]);
											close(pipeCurr[1]);
										}
										if (execvp(pipeArgs[i][0], pipeArgs[i]) == -1) {
											perror("execvp");
											return -1;
										}
									} else {
										if (i > 0) {
											close(pipePrev[0]);
											close(pipePrev[1]);
										}
										if (i < pipeCount - 1) {
											pipePrev[0] = pipeCurr[0];
											pipePrev[1] = pipeCurr[1];
										}
									}
								}
								int status;
								while (waitpid(-pgid, &status, WUNTRACED) > 0) {
									wait(NULL);
								}
								for (int i = 0; i < pipeCount; i++) {
									free(pipeArgs[i]);
								}
							} else {
								pid_t childPid = fork();
								if (childPid < 0) {
									return -1;
								} else if (childPid == 0) {
									if (execvp(curr->arguments[0], curr->arguments) == -1) {
										perror("execvp");
										exit(-1);
									}
									exit(0);
								} else {
									wait(NULL);
								}
							}
						}
					}
				}
				else if (strcmp(arguments[1], "set") == 0) {
					int newSize;
					sscanf(arguments[2], "%d", &newSize);
					if (head == NULL) {
						
					} else if (newSize == 0) {
						HISTNODE *tempFree = head;
						while (tempFree != NULL) {
							HISTNODE *next = tempFree->next;
							for (int i = 0; i < tempFree->argCount + 1; i++) {
								free(tempFree->arguments[i]);
							}
							free(tempFree->arguments);
							free(tempFree);
							tempFree= next;
						}
						head = NULL;
					}
					else if (newSize < histSize) {
						HISTNODE *curr = head;
						int currNode = 1;
						while ((currNode <= histSize) && (curr != NULL)) {
							if (curr->next == NULL) {
								break;
							}
							curr = curr->next;
							currNode++;
						}
						while (currNode > newSize) {
							HISTNODE *toFree = curr;
							curr = curr->prev;
							for (int i = 0; i < toFree->argCount; i++) {
							    free(toFree->arguments[i]);
							}
							free(toFree->arguments);
							free(toFree);
							currNode--;
						}
						curr->next = NULL;
					}
					histSize = newSize;
				}
				free(arguments[0]);
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				//free(currLine);
				free(command2);
				continue;
			}

			if (strcmp(arguments[0], "local") == 0) {
			    char *varAssign = strdup(arguments[1]);
			    VARNODE *toAdd = (VARNODE *)(malloc(sizeof(VARNODE)));
			    toAdd->name = strsep(&varAssign, "=");
			    toAdd->value = strdup(strsep(&varAssign, "\0"));
			    strcat(toAdd->value, "\0");
			    VARNODE *currVar = headVar;
			    bool conti = false;
			    while (currVar != NULL) {
			        if (strcmp(toAdd->name, currVar->name) == 0) {
			            if (strcmp(toAdd->value, "") == 0) {
			                if (currVar == headVar) {
			                    headVar = headVar->next;
			                    free(currVar);
			                } else if (currVar->next == NULL) {
			                    currVar->prev->next = NULL;
			                    free(currVar);
			                } else {
			                    currVar->prev->next = currVar->next;
			                    currVar->next->prev = currVar->prev;
			                    free(currVar);
			                }
			            } else {
			                currVar->value = toAdd->value;
			                free(toAdd);
			            }
			            conti = true;
			            break;
			        }
			        currVar = currVar->next;
			    }
			    if (conti != true) {
			        if (headVar == NULL) {
			            headVar = toAdd;
			            toAdd->next = NULL;
			            toAdd->prev = NULL;
			        } else {
			            VARNODE *end = headVar;
			            while (end->next != NULL) {
			                end = end->next;
			            }
			            end->next = toAdd;
			            toAdd->prev = end;
			            toAdd->next = NULL;
			        }
			    }
			    free(arguments[0]);
			    free(arguments);
			    free(currLineCopy1);
			    free(currLineCopy2);
			    //free(currLine);
			    free(command2);
			    continue;
			}
			
			if (strcmp(arguments[0], "vars") == 0) {
			    VARNODE *currVar = headVar;
			    while (currVar != NULL) {
			        printf("%s=%s\n", currVar->name, currVar->value);
			        currVar = currVar->next;
			    }
			    free(arguments[0]);
			    free(arguments);
			    free(currLineCopy1);
			    free(currLineCopy2);
			    //free(currLine);
			    free(command2);
			    continue;
			}

			if (strcmp(arguments[0], "export") == 0) {
			    char *varAssign = strdup(arguments[1]);
			    VARNODE *toAdd = (VARNODE *)malloc(sizeof(VARNODE));
			    toAdd->name = strsep(&varAssign, "=");
			    toAdd->value = strsep(&varAssign, "");
			    setenv(toAdd->name, toAdd->value, 1);
			    free(toAdd);
			    free(arguments[0]);
			    free(arguments);
			    free(currLineCopy1);
			    free(currLineCopy2);
			    //free(currLine);
			    free(command2);
			    continue;
			}

			if (strcmp(arguments[0], "exit") == 0) {
				if (count > 1) {
					return -1;
				}
				free(arguments[0]);
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				//free(currLine);
				free(command2);
				break;
			}

			if (strcmp(arguments[0], "cd") == 0) {
				if (count != 2) {
					return -1;
				} else {
					if (chdir(arguments[1]) != 0) {
						return -1;
					}
				}
				free(arguments[0]);
				free(arguments);
				free(currLineCopy1);
				free(currLineCopy2);
				//free(currLine);
				free(command2);
				continue;
			}
			
			if (histSize > 0) {
				HISTNODE *newHead = (HISTNODE *)(malloc(sizeof(HISTNODE)));
				newHead->arguments = malloc((count + 1) * sizeof(char*));
				newHead->arguments[0] = strdup(command);
				for (int i = 1; i < count; i++) {
					newHead->arguments[i] = strdup(arguments[i]);
				}
				newHead->arguments[count] = NULL; 
				newHead->argCount = count;
				newHead->prev = NULL;
				newHead->next = NULL;
				if (head == NULL) {
					head = newHead;
				} else {
					newHead->next = head;
					head->prev = newHead;
					head = newHead;
				}
				HISTNODE *curr = head;
				int currCount = 1;
				while (curr->next != NULL) {
					curr = curr->next;
					currCount++;
				}
				if (currCount < histSize) {
					
				} else {
					while (currCount > histSize) {
						HISTNODE *toFree = curr;
						curr = curr->prev;
						for (int i = 0; i < toFree->argCount; i++) {
						    free(toFree->arguments[i]);
						}
						free(toFree->arguments);
						free(toFree);
						currCount--;
					}
					curr->next = NULL;
				}
			}

			int pipeCount = 1;
			bool hasPipe = false;
			for (int i = 0; i < count; i++) {
				if (strcmp(arguments[i], "|") == 0) {
					hasPipe = true;
					pipeCount++;
				}
			}
			if (hasPipe) {
				int pipeArgsIndex = 0;
				char **pipeArgs[pipeCount];
				int currPipe = 0;
				for (int i = 0; i < count; i++) {
					if (strcmp(arguments[i], "|") == 0) {
						char **toAdd = (char **)(malloc(sizeof(char *) * (i - currPipe + 1)));
						for (int j = 0; j < (i - currPipe); j++) {
							toAdd[j] = arguments[currPipe + j];
						}
						toAdd[i-currPipe] = NULL;
						currPipe = i + 1;
						pipeArgs[pipeArgsIndex] = toAdd;
						pipeArgsIndex++;
					}
				}
				char **toAdd = (char **)(malloc(sizeof(char *) * (count - currPipe + 1)));
				for (int j = 0; j < (count - currPipe); j++) {
					toAdd[j] = arguments[currPipe + j];
				}
				toAdd[count - currPipe] = NULL;
				pipeArgs[pipeArgsIndex] = toAdd;	


				int pipePrev[2];
				int pipeCurr[2];

				pid_t pgid = getpid();
				
				for (int i = 0; i < pipeCount; i++) {
					if (i < pipeCount - 1) {
						if (pipe(pipeCurr) < 0) {
							return -1;
						}
					}
					pid_t childPid = fork();
					if (childPid < 0) {
						return -1;
					} else if (childPid == 0) {
						setpgid(0, pgid);
						if (i > 0) {
							dup2(pipePrev[0], 0);
							close(pipePrev[0]);
							close(pipePrev[1]);
						}
						if (i < pipeCount - 1) {
							dup2(pipeCurr[1], 1);
							close(pipeCurr[0]);
							close(pipeCurr[1]);
						}
						if (execvp(pipeArgs[i][0], pipeArgs[i]) == -1) {
							perror("execvp");
							exit(-1);
						}
					} else {
						if (i > 0) {
							close(pipePrev[0]);
							close(pipePrev[1]);
						}
						if (i < pipeCount - 1) {
							pipePrev[0] = pipeCurr[0];
							pipePrev[1] = pipeCurr[1];
						}
					}	
				}
				
				int status;
				while (waitpid(-pgid, &status, WUNTRACED) > 0) {
					wait(NULL);
				}

				for (int i = 0; i < pipeCount; i++) {
					free(pipeArgs[i]);
				}
							
			} else {		
				pid_t childPid = fork();
				if (childPid < 0) {
					return -1;
				} else if (childPid == 0) {
					if (execvp(arguments[0], arguments) == -1) {
						perror("execvp");
						exit(-1);
					}
					exit(0);
				} else {
					wait(NULL);
				}
			}
			//free(arguments[0]);
			free(arguments);
			free(currLineCopy1);
			free(currLineCopy2);
			//free(currLine);
			free(command2);
			continue;
		}
		fclose(batchFile);
	}
	HISTNODE *tempFree = head;
	while (tempFree != NULL) {
		HISTNODE *next = tempFree->next;
		for (int i = 0; i < tempFree->argCount + 1; i++) {
			free(tempFree->arguments[i]);
		}
		free(tempFree->arguments);
		free(tempFree);
		tempFree = next;
	}
	VARNODE *tempVar = headVar;
	while (tempVar != NULL) {
		VARNODE *next = tempVar->next;
		free(tempVar->name);
		free(tempVar->value);
		free(tempVar);
		tempVar = next;
	}
	return 0;
}
