#pragma once

#include <ituGL/scene/SceneNode.h>
//#include <ituGL/renderer/Renderable.h>
#include <vector>

class Model;

class SceneModel : public SceneNode//, public Renderable
{
public:
    SceneModel(const std::string& name, std::shared_ptr<Model> model, std::vector<int> drawCallCollectionIndeces);
    SceneModel(const std::string& name, std::shared_ptr<Model> model, std::shared_ptr<Transform> transform);

    std::shared_ptr<Model> GetModel() const;
    void SetModel(std::shared_ptr<Model> model);

    //glm::mat4 GetWorldMatrix() const override;
    //int GetDrawcallCount() const override;
    //const Drawcall& GetDrawcall(int index, const VertexArrayObject*& vao, const Material*& material) const override;

    SphereBounds GetSphereBounds() const override;
    AabbBounds GetAabbBounds() const override;
    BoxBounds GetBoxBounds() const override;

    void AcceptVisitor(SceneVisitor& visitor) override;
    void AcceptVisitor(SceneVisitor& visitor) const override;

    std::vector<int> GetDrawCallCollectionIndeces();

private:
    std::shared_ptr<Model> m_model;
    std::vector<int> m_drawCallCollectionIndeces;
};
