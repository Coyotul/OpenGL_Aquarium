#include "Mesh.h"

Mesh::Mesh(const vector<Vertex>& vertices, const vector<unsigned int>& indices, const vector<Texture>& textures)
{
   std::cout << "start mesh constructor " << std::endl;

   std::cout << "reserve vertices " << std::endl;
   numVertices = vertices.size();
   this->vertices.reset(new Vertex[numVertices]);

   std::cout << "start copy vertices " << std::endl;
   for (size_t i = 0; i < vertices.size(); ++i) {
      this->vertices.get()[i] = vertices[i];
   }

   std::cout << "reserve indices " << std::endl;
   numIndexes = indices.size();
   this->indices.reset(new unsigned int[numIndexes]);

   std::cout << "start copy indices " << std::endl;
   for (size_t i = 0; i < indices.size(); ++i) {
      this->indices.get()[i] = indices[i];
   }

   std::cout << "start copy textures " << std::endl;
   this->textures = textures;

   // now that we have all the required data, set the vertex buffers and its attribute pointers.
   setupMesh();

   std::cout << "end mesh constructor " << std::endl;
}

Mesh::Mesh(unsigned int numVertices, std::shared_ptr <Vertex> vertices, unsigned int numIndexes, std::shared_ptr <unsigned int> indices, const vector<Texture>& textures)
{
   std::cout << "start mesh constructor. num vertice = " << numVertices << " num indexes " << numIndexes << std::endl;

   this->numVertices = numVertices;
   this->vertices = vertices;

   this->numIndexes = numIndexes;
   this->indices = indices;

   this->textures = textures;

   // now that we have all the required data, set the vertex buffers and its attribute pointers.
   setupMesh();

   std::cout << "end mesh constructor " << std::endl;
}

// render the mesh
void Mesh::Draw(Shader& shader)
{
   //std::cout << "start drawing " << std::endl;

   // bind appropriate textures
   unsigned int diffuseNr = 1;
   unsigned int specularNr = 1;
   unsigned int normalNr = 1;
   unsigned int heightNr = 1;
   for (unsigned int i = 0; i < textures.size(); i++)
   {
      glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
      // retrieve texture number (the N in diffuse_textureN)
      string number;
      string name = textures[i].type;
      if (name == "texture_diffuse")
         number = std::to_string(diffuseNr++);
      else if (name == "texture_specular")
         number = std::to_string(specularNr++); // transfer unsigned int to string
      else if (name == "texture_normal")
         number = std::to_string(normalNr++); // transfer unsigned int to string
      else if (name == "texture_height")
         number = std::to_string(heightNr++); // transfer unsigned int to string

     // now set the sampler to the correct texture unit
      glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
      // and finally bind the texture
      glBindTexture(GL_TEXTURE_2D, textures[i].id);
   }

   // draw mesh
   glBindVertexArray(VAO);

   //std::cout << "draw triangles: " << numIndexes << std::endl;
   glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(numIndexes), GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);

   // always good practice to set everything back to defaults once configured.
   glActiveTexture(GL_TEXTURE0);
   //std::cout << "end drawing " << std::endl;
}

// initializes all the buffer objects/arrays
void Mesh::setupMesh()
{
   std::cout << "start to setup mesh " << std::endl;
   // create buffers/arrays
   glGenVertexArrays(1, &VAO);
   glGenBuffers(1, &VBO);
   glGenBuffers(1, &EBO);

   glBindVertexArray(VAO);
   // load data into vertex buffers
   glBindBuffer(GL_ARRAY_BUFFER, VBO);
   // A great thing about structs is that their memory layout is sequential for all its items.
   // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
   // again translates to 3/2 floats which translates to a byte array.
   glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), &vertices.get()[0], GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndexes * sizeof(unsigned int), &indices.get()[0], GL_STATIC_DRAW);

   // set the vertex attribute pointers
   // vertex Positions
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
   // vertex normals
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
   // vertex texture coords
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
   //// vertex tangent
   //glEnableVertexAttribArray(3);
   //glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
   //// vertex bitangent
   //glEnableVertexAttribArray(4);
   //glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
   //// ids
   //glEnableVertexAttribArray(5);
   //glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

   //// weights
   //glEnableVertexAttribArray(6);
   //glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
   glBindVertexArray(0);

   std::cout << "end to setup mesh " << std::endl;
}

