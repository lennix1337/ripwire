import { runPipeline } from './core'

// a second one-line wrapper: the follow goes on to its callee
export function forwardInput(input: Request): Promise<Response> {
  return runPipeline(input)
}
