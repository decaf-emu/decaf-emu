#include <functional>

namespace ios::internal
{

using WorkerTask = std::function<void()>;

void
startWorkerThread();

void
stopWorkerThread();

void
submitWorkerTask(WorkerTask task);

} // namespace ios::internal
