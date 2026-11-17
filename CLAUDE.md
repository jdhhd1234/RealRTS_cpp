# Modern C++20 작성규칙(필수로 읽기)

1. 스마트 포인터는 진짜 필요할때만 쓰고 가급적 사용하지 않는다.
2. std::vector을 최대한 활용한다.
3. C언어에 쓰이는 malloc, free, gets_s, scanf같은 문법을 절떄 쓰지 않는다 만약 쓴다면 STL에서 찾아서 써라.
4. template, constexpr, inline, concept는 사용 가능하다.
5. class를 쓸떄는 상속 기반 다형성, 과도한 객체지향 계층 구조, 불필요한 추상화 같은 과도한 OOP요소를 금지하며 오직 class는
```cpp
class Example
{
public:
    void Test();
    void Test2();

private:
    Example_2 example2;
};
```
와 같이 즉 의미를 명확히 하기위한 용도로만 사용이 가능하다.

6. 하나의 class가 비대하게 커지는 God class를 절떄금지한다. 
7. RAII와 Move Semantics의 사용을 강력하게 허락한다.
8. operator는 의미가 진짜 초등학생도 봤을떄 명확할정도에만 사용이 가능하며 이 외에 경우에는 절때쓰지 않는다.

# 마지막으로 가장중요한것.

무조껀 애매하다 생각하면 
예시: "초등학생도 이해할정도가 뭐지?" or "스마트 포인터를 지금 써도 되나...?"

절떄로 너 혼자 자체적으로 판단하지 말고 코드를 보여주고 나한태서 답변을 기다려라