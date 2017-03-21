/* 2D Vector Rotations example using GNU Scientific Library & CBLAS*/

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Linked list struct to hold the rotation and displacement sequences.

const double M_PI = acos(-1.0);

struct inputstuff {
	double yaw;
	double pitch;
	double roll;
	double length;
	struct input* next;
};

/*Function to fill up a linked list with yaw, pitch, roll, and displacement
values entered interactively
It should really get all the values on one line and then parse it to make it
much more convenient however
that would be more work than I want to do for this little example program.
*/

void get_list(struct inputstuff* head)
{
	int y = 0;

	struct inputstuff* current = head;
	while (y == 0) {
		printf("\nEnter in Yaw:\n");
		scanf("%lf", &current->yaw);
		printf("\nEnter in Pitch:\n");
		scanf("%lf", &current->pitch);
		printf("\nEnter in Roll:\n");
		scanf("%lf", &current->roll);
		printf("\nEnter in Length:\n");
		scanf("%lf", &current->length);
		printf("\nPress enter to enter another element or press CTRL-D to end the input.\n");

		getchar(); // Clear input buffer so we can wait for a newline or CTRL-D.

		while (1) {
			int c = getchar();
			if (c == '\n') {
				break;
			} else if (c == EOF) {
				y = 1;
				break;
			}
		}

		if (y == 0) {
			current->next = malloc(sizeof(struct inputstuff));
			current = current->next;
		}
	}
	current->next = NULL;
}

void print_list(struct inputstuff* head)
{
	struct inputstuff* current = head;
	printf("\nEntered info into the list\n");
	while (current != NULL) {
		printf("\nYaw: %2.3f, Pitch: %2.3f, Roll: %2.3f, Length: %2.3f\n", current->yaw, current->pitch, current->roll, current->length); 
		current = current->next;
	}
}

int main()
{

	struct inputstuff* head = malloc(sizeof(struct inputstuff)); // Start the linked list.

	get_list(head);

	// Set up the initial state for the rotation (identity matrix) and vector
	// displacement from the origin (0, 0)

	// The below are matrix/vector  views generated from the arrays object so the
	// underlying matrix/vector can be easily accessed to display the values.

	double current_rot_state[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
	gsl_matrix_view current_rot_state_m = gsl_matrix_view_array(current_rot_state, 3, 3);

	double current_vect_state[] = { 0, 0, 0 };
	gsl_vector_view current_vect_state_v = gsl_vector_view_array(current_vect_state, 3);

	struct inputstuff* current = head;

	while (current != NULL) {
		update_move(current, current_rot_state_m, current_vect_state_v);
		current = current->next;
	}

	printf("\nResulting rotation:\n");
	printf("\nNote, the order the rotations are applied in the order as follows: "
		   "yaw, pitch, roll.\n");
	printf("     [\t%2.3f,\t%2.3f,\t%2.3f\n", current_rot_state[0],
		current_rot_state[1], current_rot_state[2]);
	printf("\t%2.3f,\t%2.3f,\t%2.3f\n", current_rot_state[3],
		current_rot_state[4], current_rot_state[5]);
	printf("\t%2.3f,\t%2.3f,\t%2.3f  ]\n", current_rot_state[6],
		current_rot_state[7], current_rot_state[1]);

	printf("Yaw =\t%2.1f,\n",
		(180.0 / M_PI) * atan2(current_rot_state[3], current_rot_state[0]));
	printf("Pitch =\t%2.1f,\n", (180.0 / M_PI) * atan2(-current_rot_state[6], (sqrt(current_rot_state[7] * current_rot_state[7] + current_rot_state[8] * current_rot_state[8]))));
	printf("Roll =\t%2.1f,\n", (180.0 / M_PI) * atan2(current_rot_state[7], current_rot_state[8]));
	printf("\n");

	printf("Resulting displacement:\n");
	printf("     [\t%2.3f,\t%2.3f,\t%2.3f ]\n", current_vect_state[0], current_vect_state[1], current_vect_state[2]);
}

/*Note this function updates the displacement vector and rotation matrix each
time it is run. The GNU Scientific Library
BLAS functions are used to perform the matrix and vector arithmetic.

inputstuff = struct containing the yaw, pitch, roll, and displacement requested
current_rot_state_m = the the current rotation matrix prior to applying the
additional rotations requested
current_vect_state_v = the current displacement from the origin prior to
applying the additional rotations and displacement
*/

void update_move(struct inputstuff* current,
	gsl_matrix_view current_rot_state_m,
	gsl_vector_view current_vect_state_v)
{

	double displ = current->length;
	double yaw = current->yaw * M_PI / 180;
	double pitch = current->pitch * M_PI / 180;
	double roll = current->roll * M_PI / 180;

	// Initialize the matrices and vectors required for the arithmetic

	gsl_matrix* rotation = gsl_matrix_calloc(3, 3);
	gsl_matrix* last_rot_state = gsl_matrix_calloc(3, 3);
	gsl_matrix* intermediate = gsl_matrix_calloc(3, 3);
	gsl_vector* last_vector_state = gsl_vector_calloc(3);

	gsl_vector* unit_vector = gsl_vector_calloc(3);
	gsl_vector_set_basis(unit_vector, 0);

	double yaw_rot[] = { 	cos(yaw), 		-sin(yaw), 	0, 
							sin(yaw), 		cos(yaw), 	0, 
							0, 				0, 			1 };

	double pitch_rot[] = { 	cos(pitch), 	0, 			sin(pitch), 
							0, 				1,			0, 
							-sin(pitch), 	0, 			cos(pitch) };

	double roll_rot[] = {   1, 				0, 			0, 
							0, 				cos(roll), 	-sin(roll), 
							0, 				sin(roll), 	cos(roll) };

	gsl_matrix_view roll_rot_m = gsl_matrix_view_array(roll_rot, 3, 3);
	gsl_matrix_view pitch_rot_m = gsl_matrix_view_array(pitch_rot, 3, 3);
	gsl_matrix_view yaw_rot_m = gsl_matrix_view_array(yaw_rot, 3, 3);

	// Copy the current rotation and displacement so they can be used to place the
	// results into at the end.

	gsl_matrix_memcpy(last_rot_state, &current_rot_state_m.matrix);
	gsl_vector_memcpy(last_vector_state, &current_vect_state_v.vector);

	// Multiply the rotation matrices together to get the total rotation and then
	// multiply the current rotation matrix
	// by the new rotation matrix to get the total updated rotation.

	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &yaw_rot_m.matrix, &pitch_rot_m.matrix, 0.0, intermediate);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, intermediate, &roll_rot_m.matrix, 0.0, rotation);

	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, last_rot_state, rotation, 0.0, &current_rot_state_m.matrix);

	// Multiply the current rotation by the unit vector times the displacement to
	// get the change in displacement
	// and then add it to the last displacement to get the updated total
	// displacement.

	gsl_blas_dgemv(CblasNoTrans, displ, &current_rot_state_m.matrix, unit_vector, 0.0, &current_vect_state_v.vector);
	gsl_vector_add(&current_vect_state_v.vector, last_vector_state);
}





