/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CSC 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define MIN(a, b) ((a) < (b) ? (a) : (b)) 
#define MAX(a, b) ((a) > (b) ? (a) : (b)) 
#define PI 3.14159265358979323846

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Shader programs
	std::shared_ptr<Program> prog, texProg, shadow;

	//our geometry
	shared_ptr<Shape> theBunny;
	vector<shared_ptr<Shape>> comm, tower, tree, sign;

	vec3 commMin, commMax, towerMin, towerMax, treeMin, treeMax, signMin, signMax;

	//global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	
	//ground VAO
	GLuint GroundVertexArrayID;

	shared_ptr<Texture> texture0;

	//global data (larger program should be encapsulated)
	vec3 gMin;
	float gRot = 0;
	
	//camera
	float gCamH = 0;
	vec3 eye = vec3(0.0f, 0.0f, 3.0f); // position
	vec3 center = vec3(0.0f, 0.0f, -1.0f); // "target"
	vec3 up = vec3(0.0f, 1.0f, 0.0f); // right axis
	
	// time keeping
	float delta = 0.0f;
	float last = 0.0f;
	
	//animation data
	float lightTrans = 0;
	float lightY = 0;
	float gTrans = -3;
	float sTheta = 0;
	float eTheta = 0;
	float hTheta = 0;
	
	// material toggle
	bool m_toggle = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		float camSpeed = 3.0f * delta;
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			eye -= normalize(cross(center, up)) * camSpeed;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			eye += normalize(cross(center, up)) * camSpeed;
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS) {
			eye += camSpeed * center;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			eye -= camSpeed * center;
		}
		if (key == GLFW_KEY_M && action == GLFW_PRESS){
			m_toggle = !m_toggle;
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans += 0.25f;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans -= 0.25f;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}
	
	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		//code for pitch and yaw variable updates
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.855f, .886f, .89f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightColor");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");
		
		// shadow texture
		shadow = make_shared<Program>();
		shadow->setVerbose(true);
		shadow->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/shadow_frag.glsl");
		shadow->init();
		shadow->addUniform("P");
		shadow->addUniform("V");
		shadow->addUniform("M");
		shadow->addAttribute("vertPos");
		shadow->addAttribute("vertNor");

		//read in and load the texture
		texture0 = make_shared<Texture>();
  		texture0->setFilename(resourceDirectory + "/ground19.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
	}

	void initGeom(const std::string& resourceDirectory)
	{
		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information. We'll ignore them for this assignment.
		
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;

		// Initialize bunny mesh.
		vector<tinyobj::shape_t> TOshapesB;
 		vector<tinyobj::material_t> objMaterialsB;
		//load in the mesh and make the shape(s)
 		bool rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr, (resourceDirectory + "/bunnyNoNorm.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			
			theBunny = make_shared<Shape>();
			theBunny->createShape(TOshapesB[0]);
			theBunny->measure();
			theBunny->init();
		}
		
		meshLoader(resourceDirectory, comm, "/comm.obj", commMin, commMax);
		meshLoader(resourceDirectory, tower, "/tower.obj", towerMin, towerMax);
		meshLoader(resourceDirectory, tree, "/maple.obj", treeMin, treeMax);
		meshLoader(resourceDirectory, sign, "/stop.obj", signMin, signMax);
		
		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();
	}
	
	void meshLoader(const std::string& resourceDirectory, vector<shared_ptr<Shape>>& mesh, string mFile, vec3& mMin, vec3& mMax){
		vector<tinyobj::shape_t> TOshapes;
		vector<tinyobj::material_t> objMaterials;
		string errStr;
		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + mFile).c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			for (tinyobj::shape_t shapes : TOshapes){
				auto shape = make_shared<Shape>();
				shape->createShape(shapes);
				shape->measure();
				shape->init();
				
				mMin.x = MIN(mMin.x, shape->min.x);
				mMax.x = MAX(mMax.x, shape->max.x);
				mMin.y = MIN(mMin.y, shape->min.y);
				mMax.y = MAX(mMax.y, shape->max.y);
				mMin.z = MIN(mMin.z, shape->min.z);
				mMax.z = MAX(mMax.z, shape->max.z);
				
				mesh.push_back(shape);
			}
		}
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 75;
		float g_groundY = -0.25f;
		float tiles = 20.0f; // times texture repeats

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, tiles,
      		tiles, tiles,
      		tiles, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
      }

      //code to draw the ground plane
     void drawGround(shared_ptr<Program> curS) {
     	curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
     	texture0->bind(curS->getUniform("Texture0"));
		//draw the ground plane 
  		SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		curS->unbind();
     }

     //helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i) {

    	switch (i) {
    		case 0: // pink plasticky
    			glUniform3f(curS->getUniform("MatAmb"), 0.096f, 0.046f, 0.095f);
				glUniform3f(curS->getUniform("MatDif"), 0.96f, 0.46f, 0.95f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.45f, 0.23f, 0.45f);
    			glUniform1f(curS->getUniform("MatShine"), 120.0f);
    		break;
    		case 1: // chrome
    			glUniform3f(curS->getUniform("MatAmb"), 0.25f, 0.25f, 0.25f);
    			glUniform3f(curS->getUniform("MatDif"), 0.4f, 0.4f, 0.4f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.774597f, 0.774597f, 0.774597f);
    			glUniform1f(curS->getUniform("MatShine"), 76.8f);
    		break;
    		case 2: // polished copper
    			glUniform3f(curS->getUniform("MatAmb"), 0.2295f, 0.08825f, 0.0275f);
    			glUniform3f(curS->getUniform("MatDif"), 0.5508f, 0.2118f, 0.066f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.580594f, 0.223257f, 0.0695701f);
    			glUniform1f(curS->getUniform("MatShine"), 51.2f);
    		break;
			case 3: // jade
    			glUniform3f(curS->getUniform("MatAmb"), 0.135f, 0.2225f, 0.1575f);
    			glUniform3f(curS->getUniform("MatDif"), 0.54f, 0.89f, 0.63f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.316228f, 0.316228f, 0.316228f);
    			glUniform1f(curS->getUniform("MatShine"), 12.8f);
    		break;
			case 4: // emerald
    			glUniform3f(curS->getUniform("MatAmb"), 0.0215f, 0.1745f, 0.0215f);
    			glUniform3f(curS->getUniform("MatDif"), 0.07568f, 0.61424f, 0.07568f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.633f, 0.727811f, 0.633f);
    			glUniform1f(curS->getUniform("MatShine"), 76.8f);
    		break;
			case 5: // brick
    			glUniform3f(curS->getUniform("MatAmb"), 0.2f, 0.055f, 0.02f);
    			glUniform3f(curS->getUniform("MatDif"), 0.6f, 0.2f, 0.1f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.05f, 0.05f, 0.05f);
    			glUniform1f(curS->getUniform("MatShine"), 10.0f);
    		break;
			case 6: // silver
    			glUniform3f(curS->getUniform("MatAmb"), 0.19225f, 0.19225f, 0.19225f);
    			glUniform3f(curS->getUniform("MatDif"), 0.50754f, 0.50754f, 0.50754f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.508273f, 0.508273f, 0.508273f);
    			glUniform1f(curS->getUniform("MatShine"), 51.2f);
    		break;
  		}
	}

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}
	
	vec3 findCenter(vec3 min, vec3 max){
		vec3 result;
		result.x = (max.x + min.x) / 2;
		result.y = (max.y + min.y) / 2;
		result.z = (max.z + min.z) / 2;
		return result;
	}
	
	float maxExtent(vec3 min, vec3 max){
		return MAX((max.z - min.z), MAX((max.y - min.y), (max.x - min.x)));
	}
	
	void drawBunnies(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog, bool shadow){
		int material = m_toggle ? 4 : 3;
		if(!shadow){ SetMaterial(prog, material); }
		Model->pushMatrix();
		float sp = 3.0;
		float off = -3.5;
		float y_ax = shadow ? -1.23f : -1.35f;
		for (int i = 0; i < 3; i++) {
			for (int j =0; j < 3; j++) {
				Model->pushMatrix();
				Model->translate(vec3(off + sp * i, y_ax, off + sp * j));
				Model->rotate((float)(PI/4)*i+j, vec3(0, 1, 0));
				Model->scale(vec3(4, 4, 4));
				if(shadow){ Model->scale(vec3(1.0f, 0, 1.0f)); }
				setModel(prog, Model);
				theBunny->draw(prog);
				Model->popMatrix();
			}
		}
		Model->popMatrix();
	}
	
	void drawCommTower(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog){
	    SetMaterial(prog, 6);
		
		Model->pushMatrix(); // comm towers
		vec3 comm_center = findCenter(commMin, commMax);
		float comm_scale = (2.0f / maxExtent(commMin, commMax))*12;
		Model->translate(vec3(-comm_center.x, -0.65f, -comm_center.z - 40));
		Model->rotate((float)PI, vec3(0, 1, 0));
		Model->scale(vec3(comm_scale));
		setModel(prog, Model);
		
		for(int i = 0; i < comm.size(); i++){
			if (i != 6 && i!= 9){comm[i]->draw(prog);}
		}
		
		Model->pushMatrix();					// animate dishes on the comm towers
		Model->translate(vec3(10, 0, -10));
		Model->rotate(sTheta/4, vec3(0, 1, 0));
		Model->translate(vec3(-10, 0, 10));
		setModel(prog, Model);
		comm[6]->draw(prog);
		Model->popMatrix();
		
		Model->pushMatrix();
		Model->translate(vec3(-10, 0, -10));
		Model->rotate(-sTheta/4, vec3(0, 1, 0));
		Model->translate(vec3(10, 0, 10));
		setModel(prog, Model);
		comm[9]->draw(prog);
		Model->popMatrix();
		
		Model->popMatrix(); // end comm towers
	}
	
	void drawTower(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog){
		SetMaterial(prog, 5);
		Model->pushMatrix();
		vec3 t_center = findCenter(towerMin, towerMax);
		float t_scale = (2.0f / maxExtent(towerMin, towerMax))*12;
		for(int i = -6; i < 6; i++){
			Model->pushMatrix();
			Model->rotate((float)PI/2, vec3(0, 1, 0));
			Model->translate(vec3(-t_center.x+(i*13), -1.4f, -t_center.z + 8));
			Model->scale(vec3(t_scale));
			setModel(prog, Model);
			for (auto shape : tower){shape->draw(prog);}
			Model->popMatrix();
		}
		for(int i = -5; i < 7; i++){
			Model->pushMatrix();
			Model->rotate(-(float)PI/2, vec3(0, 1, 0));
			Model->translate(vec3(-t_center.x+(i*13), -1.4f, -t_center.z + 11));
			Model->scale(vec3(t_scale));
			setModel(prog, Model);
			for (auto shape : tower){shape->draw(prog);}
			Model->popMatrix();
		}
		Model->popMatrix();
	}
	
	void drawSign(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog){
		SetMaterial(prog, 1);
		Model->pushMatrix();
		vec3 t_center = findCenter(signMin, signMax);
		float t_scale = (2.0f / maxExtent(signMin, signMax))*1.25f;
		Model->rotate((float)PI/10, vec3(0, 1, 1));
		Model->translate(vec3(-t_center.x + 3, -1.7f, -t_center.z - 5));
		Model->scale(vec3(t_scale));
		setModel(prog, Model);
		for (auto shape : sign){shape->draw(prog);}
		Model->popMatrix();
	}
		
	void drawTrees(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog, bool shadow){
		int material = m_toggle ? 3 : 4;
		if(!shadow){ SetMaterial(prog, material); }
		Model->pushMatrix();
		vec3 t_center = findCenter(treeMin, treeMax);
		float tree_scale = (2.0f / maxExtent(treeMin, treeMax)) * 2.0f;
		float y_ax = shadow ? -1.24f : -1.30f;
		Model->translate(vec3(-t_center.x, y_ax, -t_center.z));
		vector<vec3> pos;
		pos.push_back(vec3(-4, 0, -4));
		pos.push_back(vec3(0, 0, -8));
		pos.push_back(vec3(3, 0, -7));
		pos.push_back(vec3(-3, 0, 6));
		pos.push_back(vec3(2, 0, 3));
		pos.push_back(vec3(2, 0, -10));
		pos.push_back(vec3(1, 0, -3));
		pos.push_back(vec3(-2, 0, 20));
		for(int i = 0; i < pos.size(); i++){
			Model->pushMatrix();
			Model->translate(pos[i]);
			Model->rotate((float)sin(i+2), vec3(0, 1, 0));
			vec3 scale = (i%2 == 0) ? vec3(tree_scale) : vec3(tree_scale * 1.45f);
			Model->scale(scale);
			if(shadow){ Model->scale(vec3(1.0f, 0, 1.0f)); }
			setModel(prog, Model);
			for (auto shape : tree){shape->draw(prog);}
			Model->popMatrix();
		}
		Model->popMatrix();
	}
	
	void render() {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);
		
		// vec3 camDir = normalize(eye - center);
		// vec3 camRight = normalize(cross(up, camDir));
		// vec3 camUp = cross(camDir, camRight);
		
		mat4 viewer = lookAt(eye, center + eye, up);
		
		View->pushMatrix();
		View->loadIdentity();
		View->multMatrix(viewer);

		// Draw the scene
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(prog->getUniform("lightPos"), 2.0f+lightTrans, 2.0f+lightY, 2.0f);
		glUniform3f(prog->getUniform("lightColor"), 1.0f, 1.0f, 1.0f);
		
		drawCommTower(Model, prog);
		drawTower(Model, prog);
		drawTrees(Model, prog, false);
		drawBunnies(Model, prog, false);
		drawSign(Model, prog);

		prog->unbind();
		
		// shadow shader
		shadow->bind();
		glUniformMatrix4fv(shadow->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(shadow->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		
		drawTrees(Model, shadow, true);
		drawBunnies(Model, shadow, true);

		shadow->unbind();
		
		//switch shaders to the texture mapping shader and draw the ground
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), 2.0f+lightTrans, 2.0f+lightY, 2.0f);
		glUniform1i(texProg->getUniform("flip"), 1);
		
		drawGround(texProg);

		texProg->unbind();
		
		//animation update example
		sTheta = (float)sin(glfwGetTime());
		eTheta = std::max(0.0f, (float)sin(glfwGetTime()));
		hTheta = std::max(0.0f, (float)cos(glfwGetTime()));

		// Pop matrix stacks.
		Projection->popMatrix();
		View->popMatrix();
		
		float current = glfwGetTime();
		delta = current - last;
		last = current;

	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
